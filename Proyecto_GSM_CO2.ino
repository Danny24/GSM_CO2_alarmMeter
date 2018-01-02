#include "SIM900.h"
#include <SoftwareSerial.h>
#include "call.h"
#include "sms.h"

SMSGSM sms;
 
CallGSM call;
boolean started=false;


unsigned long t_anterior,tiempo_cronometrado,ultimomensaje;
int ADVal, Tc_100, whole, fract,CO2; //variables globales para lectura de sensores. 
const int lastsms = 300; //tiempo (en segundos) entre alertas SMS y verifiacion para encendido/apagado del ventilador.

char number[20]; //guarda aqui el numero de origen de la llamada recibida para telemetria a peticion.
char numbersaved[20] = "3310171320"; //Numero de telefono al cual se envian las alertas SMS, para el caso de telemetria a peticion se envian al numero del cual se hizo la consulta. 

//Parametros peligrosos para enviar alertas:
const byte temp1 = 30;
const byte temp2 = 35;
const byte temp3 = 40;
const byte temp4 = 50;

const int ppm1 = 1000;
const int ppm2 = 2000;
const int ppm3 = 3000;
const int ppm4 = 5000;
/*
El*CO2 es un gas presente en la atmósfera de forma natural en una concentración de 250 a 350 ppm.
35*De 0 a 1000 ppm es la concentración de calidad aceptable en un recinto cerrado. Aquí es donde estamos. Recinto cerrado.
De*De 1000 a 2000 ppm, la calidad del aire es considerada baja.
De*De 2000 ppm a 5000 empieza a causar problemas (dolor de cabeza, insomnio, naúseas). Es aire viciado.
*A partir de 5,000 ppm alteran la presencia de otros gases presentes en el aire, creándose una atmósfera tóxica o deficiente en oxígeno de consecuencias fatales según incrementa la concentración.
*/

void setup()
{
    Serial.begin(9600);
    analogReference(DEFAULT); //Referencia analogica a 5v. 
    pinMode(10,INPUT); //Para leer sensor PIR, no implementado. Sin embargo el codigo ya esta hecho.
    pinMode(A0,OUTPUT); 
    pinMode(A2,OUTPUT);
    pinMode(A1, INPUT); //Para leer sensor de temperatura MCP9701A
    pinMode(A4, INPUT); //Para leer sensor de CO2 MG811
    pinMode(11,OUTPUT); //salida para relay
    digitalWrite(A0,HIGH); //Alimentacion de VCC para sensor de temperatura
    digitalWrite(A2,LOW);  //Alimentacion de GND para sensor de temperatura
    digitalWrite(11,HIGH); //deja el relay apagado al iniciar

       
    ultimomensaje = lastsms; //inicializa el tiempo para alertas SMS
    
    if (gsm.begin(9600)) 
    {
        Serial.println("\nstatus=READY"); //Si el modulo GSM esta listo continua e imprime mensaje de bienvenida.
        started=true;
                
        //sms.SendSMS(numbersaved,"Sistema de seguridad contra Asfixia en vehiculos operativo y listo para funcionar."); //envia SMS de que el sistema esta listo. 
        Serial.println("Sistema de seguridad contra Asfixia en vehiculos operativo y listo para funcionar.");
    } 
    else 
        Serial.println("\nstatus=IDLE");
}

void loop()
{
 //PIR(); funcion para contar tiempo desde ultimo movimiento con PIR anulado por falta de RAM en arduino UNO.
 char sms_text[160]; 
 String smsText =""; //para guardar SMS a enviar segun sea el caso. 

 //int tiempo = tiempo_cronometrado/1000;

  switch (call.CallStatus())
  {
    case CALL_NONE: // No hay llamada entrante. Por lo tanto verificar sensores, servicio de alertas y ventilador se ejecutan aqui. 
      temp(); //actualizar temperatura.
      co2(); //actualizar nivel de CO2
      Serial.println("La temperatura es de: "+String(whole)+"."+String(fract)+" C. Y el nivel de CO2 es de: "+String(CO2)+" PPM.");
      //Serial.println("El ultimo movimiento detectado fue hace: "+String(tiempo)+" segundos. \n");      
      
      delay(250);
      
      if (((millis()/1000)-ultimomensaje)>= lastsms) //Si ya ha transcurido el tiempo establecido desde la ultima alerta vueleve a ejecutar las alertas, esto con el fin de enviar multiples SMS en un corto perido de tiempo.
      {
       if ((whole >= temp1 && whole <= temp2 )|| (CO2 >= ppm1 && CO2 <= ppm2))
       { 
         delay(100);
         Serial.println("Alerta nivel 1. Las condiciones en el vehiculo son apenas aceptables.");
         sms.SendSMS(numbersaved,"Alerta nivel 1. Las condiciones en el vehiculo son apenas aceptables.");
         ultimomensaje = millis()/1000;
         digitalWrite(11,LOW); //enciende ventilador
       }
       else
       {
        digitalWrite(11,HIGH); //apaga ventilador
       }
       if ((whole >= temp2 && whole <= temp3 )|| (CO2 >= ppm2 && CO2 <= ppm3))
       { 
         delay(100);
         Serial.println("Alerta nivel 2. Las condiciones en el vehiculo son deficientes");
         sms.SendSMS(numbersaved,"Alerta nivel 2. Las condiciones en el vehiculo son deficientes");
         ultimomensaje = millis()/1000;
         digitalWrite(11,LOW); //enciende ventilador
       }
       if ((whole >= temp3 && whole <= temp4 )|| (CO2 >= ppm3 && CO2 <= ppm4))
       { 
         delay(100);
         Serial.println("Alerta nivel 3. Las condiciones en el vehiculo son inaceptables");
         sms.SendSMS(numbersaved,"Alerta nivel 3. Las condiciones en el vehiculo son inaceptables");
         ultimomensaje = millis()/1000;
         digitalWrite(11,LOW); //enciende ventilador
       }
       if ((whole >= temp4 )|| (CO2 >= ppm4 ))
       { 
         delay(100);
         Serial.println("Alerta nivel 4. Las condiciones en el vehiculo peligrosas.");
         sms.SendSMS(numbersaved,"Alerta nivel 4. Las condiciones en el vehiculo peligrosas.");
         ultimomensaje = millis()/1000; 
         digitalWrite(11,LOW);//enciende ventilador
       }
     
      }
      
    break;

    case CALL_INCOM_VOICE : // Si alguien nos llama contestamos y colgamos. Inmediatamente enviamos SMS con los datos actuales de temperatura y CO2.

       Serial.println("Llamada entrante");
       call.CallStatusWithAuth(number,0,0);
       Serial.println(number);
       delay(100);
       temp();
       co2();
       delay(2000);
       call.HangUp();
       smsText = "STP - La temperatura es de: "+String(whole)+"."+String(fract)+" C. y el nivel de CO2 es de: "+String(CO2)+" PPM.";
       smsText.toCharArray(sms_text,160);
       Serial.println(smsText);
       Serial.println("");
       delay(2000);
       sms.SendSMS(number,sms_text);
      break;

    
  }
  delay(1000);
}

void temp() //calcula la temperatura en °C
{
 analogReference(DEFAULT); //Referencia analogica a 5v. 
 ADVal = analogRead(1);
 ADVal = analogRead(1); //limpieza del ADC por cambio en la referencia.
 delay(100);
 ADVal = analogRead(1);
 delay(1000);
 ADVal = analogRead(1); //leer ADC
 delay(100);
 Tc_100 = 20 * ADVal - 2050; //poner 25 para alimentacion por DC jack y 20 para alimentacion por USB.
 whole = Tc_100 / 100;
 fract = Tc_100 % 100;
 if (fract < 10)
 {
  fract = 0;
 } 
}

void co2() //calcula el nivel de CO2 en PPM. 
{
 int voltage;
 analogReference(INTERNAL); //Referencia analogica a 1v1. 
 ADVal = analogRead(4);
 ADVal = analogRead(4); //limpieza del ADC por cambio en la referencia.
 delay(100);
 ADVal = analogRead(4);
 delay(1000);
 ADVal = analogRead(4); //leer ADC
 delay(100);
 voltage = ((1.1*ADVal)*0.99);
 CO2 = map(voltage,0,1000,0,10000);
}

/*
void PIR()
{

if (digitalRead(10) == 0)  //cuento tiempo desde ultimo disparo por detecccion de movimiento
{
 tiempo_cronometrado=millis()-t_anterior;
}
else
{
 t_anterior=millis(); 
}
}
*/
