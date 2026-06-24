# -*- coding: utf-8 -*-
import logging
import json
import boto3
from boto3.dynamodb.conditions import Key
from botocore.exceptions import ClientError

import ask_sdk_core.utils as ask_utils
from ask_sdk_core.skill_builder import SkillBuilder
from ask_sdk_core.dispatch_components import AbstractRequestHandler, AbstractExceptionHandler

logger = logging.getLogger(__name__)
logger.setLevel(logging.INFO)

# =====================================================================
# CAPA DE INFRAESTRUCTURA (ACCESO A DATOS)
# =====================================================================

class CloudRepository:
    def __init__(self):
        self.dynamodb = boto3.resource('dynamodb')
        self.user_table = self.dynamodb.Table('user_devices')
        self.events_table = self.dynamodb.Table('speed_events')

    def get_latest_speed_event(self, device_id):
        try:
            response = self.events_table.query(
                KeyConditionExpression=Key('device_id').eq(device_id),
                ScanIndexForward=False,
                Limit=1
            )
            items = response.get('Items', [])
            return items[0] if items else None
        except ClientError as e:
            logger.error(f"DynamoDB Error (speed_events): {e}", exc_info=True)
            return None

class DeviceShadowClient:
    def __init__(self):
        self.iot_client = boto3.client('iot-data')

    def get_reported_state(self, device_id):
        try:
            response = self.iot_client.get_thing_shadow(thingName=device_id)
            payload = json.loads(response['payload'].read())
            return payload.get('state', {}).get('reported', {})
        except ClientError as e:
            logger.error(f"IoT Fetch Error: {e}", exc_info=True)
            return None

    def update_desired_state(self, device_id, target_key, target_value):
        payload = {"state": {"desired": {target_key: target_value}}}
        try:
            self.iot_client.update_thing_shadow(
                thingName=device_id,
                payload=json.dumps(payload)
            )
            return True
        except ClientError as e:
            logger.error(f"IoT Update Error: {e}", exc_info=True)
            return False

repository = CloudRepository()
shadow_client = DeviceShadowClient()

# =====================================================================
# CAPA DE APLICACIÓN (LÓGICA DE NEGOCIO Y REUSABILIDAD)
# =====================================================================

def resolve_device_or_fail(handler_input):
    """
    Identifica de manera unívoca la placa virtual solicitada mapeando 
    la cuenta de Alexa y el slot conversacional.
    """
    user_id = handler_input.request_envelope.session.user.user_id
    
    try:
        slots = handler_input.request_envelope.request.intent.slots
        device_slot = slots["dispositivo"].value if (slots and "dispositivo" in slots) else None
    except AttributeError:
        device_slot = None
    
    target_name = device_slot.lower().strip() if device_slot else "principal"

    try:
        user_table = boto3.resource('dynamodb').Table('user_devices')
        response = user_table.query(
            KeyConditionExpression=Key('user_id').eq(user_id) & Key('device_name').eq(target_name)
        )
        items = response.get('Items', [])
        
        if not items:
            error_msg = f"No encontré ningún dispositivo registrado como {target_name}."
            return None, handler_input.response_builder.speak(error_msg).ask("¿A qué punto de control te refieres?").response
            
        return items[0].get('device_id'), None
        
    except ClientError as e:
        logger.error(f"DynamoDB Resolution Error: {e}", exc_info=True)
        error_msg = "Error interno de autenticación en la base de datos."
        return None, handler_input.response_builder.speak(error_msg).response

def toggle_device_state(handler_input, shadow_key, speech_on, speech_off):
    """
    Manejador maestro con lógica flip-flop para invertir variables booleanas en el Shadow.
    """
    device_id, error_response = resolve_device_or_fail(handler_input)
    if error_response: return error_response

    current_state = shadow_client.get_reported_state(device_id)
    if current_state is None:
        speech = "El dispositivo solicitado no se encuentra respondiendo en la red."
        return handler_input.response_builder.speak(speech).ask("¿Deseas reintentar?").response

    new_value = not current_state.get(shadow_key, False)
    success = shadow_client.update_desired_state(device_id, shadow_key, new_value)
    
    if success:
        speech = f"{speech_on if new_value else speech_off} ¿Deseas realizar otra acción?"
    else:
        speech = "Fallo de comunicación en la infraestructura de AWS IoT."
        
    return handler_input.response_builder.speak(speech).ask("¿Algo más?").response

# =====================================================================
# CAPA DE ADAPTADORES (ALEXA HANDLERS CLEAN)
# =====================================================================

class LaunchRequestHandler(AbstractRequestHandler):
    def can_handle(self, handler_input):
        return ask_utils.is_request_type("LaunchRequest")(handler_input)
    def handle(self, handler_input):
        user_id = handler_input.request_envelope.session.user.user_id
        logger.info(f"SESION_INICIADA_USER: {user_id}")
        speech = "Sistema de control de acceso vial iniciado. Puedes consultar velocidades, modificar límites, operar barreras o verificar estados. ¿Qué deseas hacer?"
        return handler_input.response_builder.speak(speech).ask(speech).response

class LastSpeedIntentHandler(AbstractRequestHandler):
    def can_handle(self, handler_input):
        return ask_utils.is_intent_name("LastSpeedIntent")(handler_input)
    def handle(self, handler_input):
        device_id, error_response = resolve_device_or_fail(handler_input)
        if error_response: return error_response

        last_event = repository.get_latest_speed_event(device_id)
        if last_event:
            speed = last_event.get('speed', 0)
            exceeded = "excediendo el límite fijado." if last_event.get('exceeded') else "dentro de los parámetros seguros."
            speech = f"El último vehículo registrado cruzó a {speed} kilómetros por hora, {exceeded} ¿Algo más?"
        else:
            speech = "No se registran eventos de telemetría previos para esta vía."
        return handler_input.response_builder.speak(speech).ask("¿Algo más?").response

class ChangeSpeedLimitIntentHandler(AbstractRequestHandler):
    def can_handle(self, handler_input):
        return ask_utils.is_intent_name("ChangeSpeedLimitIntent")(handler_input)
    def handle(self, handler_input):
        device_id, error_response = resolve_device_or_fail(handler_input)
        if error_response: return error_response

        slots = handler_input.request_envelope.request.intent.slots
        new_limit = slots["limite"].value if "limite" in slots else None

        if not new_limit:
            speech = "Por favor, define explícitamente el valor numérico del nuevo límite."
            return handler_input.response_builder.speak(speech).ask(speech).response

        success = shadow_client.update_desired_state(device_id, "speedLimit", int(new_limit))
        speech = f"Límite regulatorio actualizado a {new_limit} kilómetros por hora." if success else "Error al procesar el cambio de límite."
        return handler_input.response_builder.speak(speech).ask("¿Algo más?").response

class SystemCheckIntentHandler(AbstractRequestHandler):
    def can_handle(self, handler_input):
        return ask_utils.is_intent_name("SystemCheckIntent")(handler_input)
    def handle(self, handler_input):
        device_id, error_response = resolve_device_or_fail(handler_input)
        if error_response: return error_response

        state = shadow_client.get_reported_state(device_id)
        if state:
            status = state.get('systemStatus', 'desconocido')
            alarm = state.get('alarm', False)
            alarm_msg = "Alerta activa por infracción." if alarm else "Operación normal sin alertas."
            speech = f"Módulo en línea. Diagnóstico: {status}. Estado actual: {alarm_msg} ¿Qué más deseas hacer?"
        else:
            speech = "El hardware seleccionado se encuentra desconectado de los servicios cloud."
        return handler_input.response_builder.speak(speech).ask("¿Qué más deseas consultar?").response

class OpenBarrierIntentHandler(AbstractRequestHandler):
    def can_handle(self, handler_input):
        return ask_utils.is_intent_name("OpenBarrierIntent")(handler_input)
    def handle(self, handler_input):
        return toggle_device_state(
            handler_input, 
            shadow_key="barrier", 
            speech_on="Comando enviado. Abriendo barrera de acceso.", 
            speech_off="Comando enviado. Asegurando y cerrando barrera."
        )

class BlinkDangerIntentHandler(AbstractRequestHandler):
    def can_handle(self, handler_input):
        return ask_utils.is_intent_name("BlinkDangerIntent")(handler_input)
    def handle(self, handler_input):
        return toggle_device_state(
            handler_input, 
            shadow_key="alarm", 
            speech_on="Modo de emergencia activado. Alarma sonora y visual en ejecución.", 
            speech_off="Sistema de alarma restablecido a la normalidad."
        )

class CancelOrStopIntentHandler(AbstractRequestHandler):
    def can_handle(self, handler_input):
        return (ask_utils.is_intent_name("AMAZON.CancelIntent")(handler_input) or
                ask_utils.is_intent_name("AMAZON.StopIntent")(handler_input))
    def handle(self, handler_input):
        return handler_input.response_builder.speak("Cerrando pasarela de control. Hasta pronto.").response

class CatchAllExceptionHandler(AbstractExceptionHandler):
    def can_handle(self, handler_input, exception):
        return True
    def handle(self, handler_input, exception):
        logger.error(f"Critical Exception Handler: {exception}", exc_info=True)
        return handler_input.response_builder.speak("Error de procesamiento interno en la nube AWS.").ask("Por favor, repite el comando.").response

# =====================================================================
# CONFIGURACIÓN Y ENRUTAMIENTO DEL SKILL BUILDER
# =====================================================================
sb = SkillBuilder()

sb.add_request_handler(LaunchRequestHandler())
sb.add_request_handler(LastSpeedIntentHandler())
sb.add_request_handler(ChangeSpeedLimitIntentHandler())
sb.add_request_handler(SystemCheckIntentHandler())
sb.add_request_handler(OpenBarrierIntentHandler())
sb.add_request_handler(BlinkDangerIntentHandler())
sb.add_request_handler(CancelOrStopIntentHandler())
sb.add_exception_handler(CatchAllExceptionHandler())

lambda_handler = sb.lambda_handler()