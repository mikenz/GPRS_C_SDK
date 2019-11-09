
extern "C" {
    bool i2c_init(void);
    void OLED_Init(void);
    void OLED_Fill(unsigned char fill_Data);
    void OLED_CLS(void);
    void OLED_ON(void);
    void OLED_OFF(void);
    void OLED_ShowStr(unsigned char x, unsigned char y, char ch[], unsigned char TextSize);
    void OLED_ShowCN(unsigned char x, unsigned char y, unsigned char N);
    void OLED_DrawBMP(unsigned char x0,unsigned char y0,unsigned char x1,unsigned char y1,unsigned char BMP[]);
};

#include <DallasTemperature.h>
#include <OneWire.h>
#include <api_debug.h>
#include <api_event.h>
#include <api_network.h>
#include <api_os.h>
#include <api_gps.h>
#include <api_socket.h>
#include <stdio.h>
#include <string.h>
#include <stdio.h>
#include <string.h>
#include <api_hal_gpio.h>
#include <api_hal_pm.h>
#include <api_ssl.h>
#include <stdbool.h>
#include <stdint.h>
#include <api_hal_i2c.h>
#include "i2c_oled.h"
#include <time.h>
#include "buffer.h"
#include "gps_parse.h"
#include "math.h"
#include "gps.h"


#define MAIN_TASK_STACK_SIZE (2048 * 2)
#define MAIN_TASK_PRIORITY 0
#define MAIN_TASK_NAME "Main Test Task"

#define SECOND_TASK_STACK_SIZE (2048 * 2)
#define SECOND_TASK_PRIORITY 1
#define SECOND_TASK_NAME "Second Test Task"

#define TIMEOUT_TASK_STACK_SIZE (2048 * 2)
#define TIMEOUT_TASK_PRIORITY 1
#define TIMEOUT_TASK_NAME "Timeout Task"

static HANDLE mainTaskHandle = NULL;
static HANDLE secondTaskHandle = NULL;
static HANDLE timeoutTaskHandle = NULL;
static HANDLE semNetworkConnected = NULL;
bool isGpsOn = true;

const char *ca_cert = "-----BEGIN CERTIFICATE-----\n\
MIIFZTCCBE2gAwIBAgISAyfWP5NDiE41PjlWZsZIjaExMA0GCSqGSIb3DQEBCwUA\n\
MEoxCzAJBgNVBAYTAlVTMRYwFAYDVQQKEw1MZXQncyBFbmNyeXB0MSMwIQYDVQQD\n\
ExpMZXQncyBFbmNyeXB0IEF1dGhvcml0eSBYMzAeFw0xOTA5MDExMDM0MThaFw0x\n\
OTExMzAxMDM0MThaMBgxFjAUBgNVBAMTDW9jZWFuc3dpbXMubnowggEiMA0GCSqG\n\
SIb3DQEBAQUAA4IBDwAwggEKAoIBAQCz6MeGYc91tAxS6jwgsTalp22MMacYMpzf\n\
odTM+msFvEzVI48F6WO+USHN/Zwajuc8XdG+eR9BCTDAYOVwkxcK96AeT9KwvjXD\n\
Bf0RT2kDPg6B790kocGI/DB41vvmL3IrFkexfHtOCw0KOt7nQLSr8lNtfT24FcNu\n\
NcjTwMW0tfwKuJDXb85hlF32Llly2bUrAHvVolu66mWkxGErEU+drJ35x6ONW59b\n\
ABP9FyBdZNHsC2lEWqrGqZr/xq9d0jUayLcSbVVexMMjRlgxWRd2UVfilq6BTBpw\n\
WVrF1FSJtVCRSIFWrMeDDmuh2duNrVpnDqKkZtYDTA9ZkAaZ/zr1AgMBAAGjggJ1\n\
MIICcTAOBgNVHQ8BAf8EBAMCBaAwHQYDVR0lBBYwFAYIKwYBBQUHAwEGCCsGAQUF\n\
BwMCMAwGA1UdEwEB/wQCMAAwHQYDVR0OBBYEFMxpdSAzPd0Y+od9vOUIGUfMkixy\n\
MB8GA1UdIwQYMBaAFKhKamMEfd265tE5t6ZFZe/zqOyhMG8GCCsGAQUFBwEBBGMw\n\
YTAuBggrBgEFBQcwAYYiaHR0cDovL29jc3AuaW50LXgzLmxldHNlbmNyeXB0Lm9y\n\
ZzAvBggrBgEFBQcwAoYjaHR0cDovL2NlcnQuaW50LXgzLmxldHNlbmNyeXB0Lm9y\n\
Zy8wKwYDVR0RBCQwIoINb2NlYW5zd2ltcy5ueoIRd3d3Lm9jZWFuc3dpbXMubnow\n\
TAYDVR0gBEUwQzAIBgZngQwBAgEwNwYLKwYBBAGC3xMBAQEwKDAmBggrBgEFBQcC\n\
ARYaaHR0cDovL2Nwcy5sZXRzZW5jcnlwdC5vcmcwggEEBgorBgEEAdZ5AgQCBIH1\n\
BIHyAPAAdgDiaUuuJujpQAnohhu2O4PUPuf+dIj7pI8okwGd3fHb/gAAAWzsmrdX\n\
AAAEAwBHMEUCIQCkCA2DRe+HNSTXE3SyXq4Zuxwb3iOfKu9kwi/NznD0mgIgKVXn\n\
/1vVkBfpY84EIXptsOMyig+fl4t0v9uU72Wz0rwAdgBj8tvN6DvMLM8LcoQnV2sz\n\
pI1hd4+9daY4scdoVEvYjQAAAWzsmrd4AAAEAwBHMEUCIHAhDDPzxuwhMO0eQsf8\n\
FvphaDN2nMJ9sWlRvZp3FB5IAiEA/g/q0waIyqeMawY9nOgqOBvNVAeWc2/mWS25\n\
X23EXx8wDQYJKoZIhvcNAQELBQADggEBAEuMGWQfvsRGA6TpAARiqp0srMElAwK9\n\
pfHPCn2hoOWZ0Tp98riaQxBQzg061JHzV4/Oos8IXAMVcF7M3mDbeZBDYjO9xOAv\n\
vvmXRDekkTy4iyTj67bPtKFx7cv6G5ZbSasmsuaortS+lmqR6OJRmhBhCBbS0Gga\n\
LhKKiIwLFXl51qHhVgXabHy/M/C1pRUpoaAPxWy/k8WlWGJoWPqSUCE7J3R4D/c0\n\
HXL4gGhp0hyAckswBYZ/UJoAIOYAORlUqq0VdPsk2icVeH6LNoLQFgTA860m4JDB\n\
CnCnGi6Nub8Vd8BXRm2b83tERPpCYgaKi0HcLVz1UIqvgjy+dyVWduk=\n\
-----END CERTIFICATE-----";

// https POST
int Https_Post(const char *domain,
               const char *port,
               const char *path,
               const char *data)
{
    uint8_t buffer[2048];
    const int lenBuffer = sizeof(buffer);
    char ip[16];
    int ret = 0;

    SSL_Error_t error;
    SSL_Config_t *config = (SSL_Config_t *)OS_Malloc(sizeof(SSL_Config_t));
    if (config == NULL)
    {
        Trace(1, "ssl not enough memory");
        return -1;
    }
    config->caCert = ca_cert;
    config->caCrl = NULL;
    config->clientCert = NULL;
    config->clientKey = NULL;
    config->clientKeyPasswd = NULL;
    config->hostName = domain;
    config->minVersion = SSL_VERSION_SSLv3;
    config->maxVersion = SSL_VERSION_TLSv1_2;
    config->verifyMode = SSL_VERIFY_MODE_NONE; // SSL_VERIFY_MODE_REQUIRED
    config->entropyCustom = "GPRS";

    Trace(1, "start dns lookup");
    ret = DNS_GetHostByName2((uint8_t *)domain, (uint8_t *)ip);
    if (ret < 0)
    {
        Trace(1, "dns error");
        SSL_Destroy(config);
        OS_Free(config);
        return -1;
    }
    Trace(1, "DNS len:%d, domain:%s, ip:%s", ret, domain, ip);

    Trace(1, "start ssl init");
    error = SSL_Init(config);
    if (error != SSL_ERROR_NONE)
    {
        Trace(1, "ssl init error:%d", error);
        SSL_Destroy(config);
        OS_Free(config);
        return -1;
    }
    Trace(1, "ssl init success");

    // Build the package
    snprintf(
        (char *)buffer, lenBuffer,
        "POST %s HTTP/1.1\r\nHost: %s\r\nContent-Type: "
        "application/x-www-form-urlencoded\r\nContent-Length: %d\r\n%s\r\n\r\n",
        path, domain, strlen(data), data);
    uint8_t *pData = buffer;
    Trace(1, "Package: %s", pData);

    // Connect to server
    error = SSL_Connect(config, ip, port);
    if (error != SSL_ERROR_NONE)
    {
        Trace(1, "ssl connect error:%d", error);
        Trace(1, "ssl destroy");
        SSL_Destroy(config);
        OS_Free(config);
        return -1;
    }

    // Send package
    Trace(1, "Write len:%d data:%s", strlen((char *)pData), pData);
    ret = SSL_Write(config, pData, strlen((char *)pData), 5000);
    if (ret <= 0)
    {
        Trace(1, "ssl write fail:%d", ret);
        Trace(1, "ssl close");
        SSL_Close(config);
        SSL_Destroy(config);
        OS_Free(config);
        return -1;
    }

    // Read response
    memset(buffer, 0, lenBuffer);
    ret = SSL_Read(config, buffer, lenBuffer, 2000);
    if (ret <= 0)
    {
        Trace(1, "ssl read fail:%d", ret);
        Trace(1, "ssl close");
        SSL_Close(config);
        SSL_Destroy(config);
        OS_Free(config);
        return -1;
    }
    Trace(1, "read len:%d, data:%s", ret, buffer);

    SSL_Close(config);
    SSL_Destroy(config);
    OS_Free(config);
    return 0;
}

void EventDispatch(const API_Event_t *pEvent)
{
    switch (pEvent->id)
    {
    case API_EVENT_ID_NO_SIMCARD:
        Trace(10, "!!NO SIM CARD%d!!!!", pEvent->param1);
        break;

    case API_EVENT_ID_NETWORK_REGISTER_SEARCHING:
        Trace(10, "network register searching");
        break;

    case API_EVENT_ID_NETWORK_REGISTER_DENIED:
        Trace(10, "network register denied");
        break;

    case API_EVENT_ID_NETWORK_REGISTER_NO:
        Trace(10, "network register no");
        break;

    case API_EVENT_ID_SYSTEM_READY:
        Trace(10, "system initialize complete");
        break;

    case API_EVENT_ID_NETWORK_REGISTERED_HOME:
    case API_EVENT_ID_NETWORK_REGISTERED_ROAMING:
    {
        uint8_t status;
        Trace(10, "network register success");
        bool ret = Network_GetAttachStatus(&status);
        if (!ret)
        {
            Trace(1, "get attach staus fail");
        }
        Trace(1, "attach status:%d", status);
        if (status == 0)
        {
            ret = Network_StartAttach();
            if (!ret)
            {
                Trace(1, "network attach fail");
            }
        }
        else
        {
            Network_PDP_Context_t context;
            strncpy(context.apn, "vodafone", sizeof(context.apn));
            strncpy(context.userName, "", sizeof(context.userName));
            strncpy(context.userPasswd, "", sizeof(context.userPasswd));
            Trace(1, "Network_StartActive 1");
            Network_StartActive(context);
        }
        break;
    }

    case API_EVENT_ID_NETWORK_ATTACHED:
        Trace(10, "network attach success");
        Network_PDP_Context_t context;
        strncpy(context.apn, "vodafone", sizeof(context.apn));
        strncpy(context.userName, "", sizeof(context.userName));
        strncpy(context.userPasswd, "", sizeof(context.userPasswd));
        Trace(1, "Network_StartActive 2");
        Network_StartActive(context);
        break;

    case API_EVENT_ID_DNS_SUCCESS:
        Trace(1, "DNS get ip address from domain success(event),domain:%s,ip:%s",
              pEvent->pParam2, pEvent->pParam1);
        break;

    case API_EVENT_ID_DNS_ERROR:
        Trace(1, "DNS get ip address error(event)!!!");
        break;

    case API_EVENT_ID_NETWORK_DEREGISTER:
        Trace(10, "API_EVENT_ID_NETWORK_DEREGISTER");
        break;
    case API_EVENT_ID_NETWORK_DETACHED:
        Trace(10, "API_EVENT_ID_NETWORK_DETACHED");
        break;
    case API_EVENT_ID_NETWORK_ATTACH_FAILED:
        Trace(10, "API_EVENT_ID_NETWORK_ATTACH_FAILED");
        break;
    case API_EVENT_ID_NETWORK_DEACTIVED:
        Trace(10, "API_EVENT_ID_NETWORK_DEACTIVED");
        break;
    case API_EVENT_ID_NETWORK_ACTIVATE_FAILED:
        Trace(10, "API_EVENT_ID_NETWORK_ACTIVATE_FAILED");
        break;
    case API_EVENT_ID_NETWORK_GOT_TIME:
        Trace(10, "API_EVENT_ID_NETWORK_GOT_TIME");
        break;
    case API_EVENT_ID_NETWORK_CELL_INFO:
        Trace(10, "API_EVENT_ID_NETWORK_CELL_INFO");
        break;
    case API_EVENT_ID_NETWORK_AVAILABEL_OPERATOR:
        Trace(10, "API_EVENT_ID_NETWORK_AVAILABEL_OPERATOR");
        break;

    case API_EVENT_ID_NETWORK_ACTIVATED:
        Trace(10, "network activate success");
        OS_ReleaseSemaphore(semNetworkConnected);
        break;

    case API_EVENT_ID_GPS_UART_RECEIVED:
        // Trace(1,"received GPS data,length:%d, data:%s",pEvent->param1,pEvent->pParam1);
        GPS_Update(pEvent->pParam1,pEvent->param1);
        break;

    case API_EVENT_ID_UART_RECEIVED:
        Trace(10, "API_EVENT_ID_UART_RECEIVED");
        break;

    default:
        break;
    }
}

// Trigger a shutdown of the power to everything after 40 seconds
// of operation to avoid excessive battery drain in the event of a
// recuring failure or timeouts.
void TimeoutTask(void *pData)
{
    GPIO_config_t *gpioConfig = (GPIO_config_t *)OS_Malloc(sizeof(GPIO_config_t));
    gpioConfig->mode = GPIO_MODE_OUTPUT;
    gpioConfig->pin = GPIO_PIN27;
    gpioConfig->defaultLevel = GPIO_LEVEL_HIGH;
    GPIO_Init(*gpioConfig);
    gpioConfig->defaultLevel = GPIO_LEVEL_LOW;
    gpioConfig->pin = GPIO_PIN28;
    GPIO_Init(*gpioConfig);
    gpioConfig->pin = GPIO_PIN14;
    GPIO_Init(*gpioConfig);

    Trace(1, "Timeout watchdog started");
    OS_Sleep(1000 * 60);
    Trace(1, "Timeout watchdog exceded, shutting down");

    // Set 'DONE' pin high
    OS_Sleep(1000);
    GPIO_Set(GPIO_PIN14, GPIO_LEVEL_HIGH);
    OS_Sleep(1000 * 10);
}

void SecondTask(void *pData)
{
    // Get time from GSM netowrk
    TIME_SetIsAutoUpdateRtcTime(true);

    // Wait for GPS
    GPS_Info_t* gpsInfo = Gps_GetInfo();
    GPS_Init();
    GPS_Open(NULL);
    unsigned int maxWait = 30;
    while(gpsInfo->rmc.latitude.value == 0 ||
          (gpsInfo->gsa[0].fix_type > gpsInfo->gsa[1].fix_type ? gpsInfo->gsa[0].fix_type : gpsInfo->gsa[1].fix_type) < 2) {
        Trace(2, "GPS fix mode:%d, BDS fix mode:%d, fix quality:%d, satellites tracked:%d",
              gpsInfo->gsa[0].fix_type,
              gpsInfo->gsa[1].fix_type,
              gpsInfo->gga.fix_quality,
              gpsInfo->gga.satellites_tracked);
        OS_Sleep(1000);
        maxWait--;
        if (!maxWait) {
            break;
        }
    }
    OS_Sleep(1000);
    Trace(2, "GPS fix mode:%d, BDS fix mode:%d, fix quality:%d, satellites tracked:%d",
            gpsInfo->gsa[0].fix_type,
            gpsInfo->gsa[1].fix_type,
            gpsInfo->gga.fix_quality,
            gpsInfo->gga.satellites_tracked);

    char postData[250];
    const int postDataLen = sizeof(postData);

    // Wait for the GPRS network connection to be available
    OS_WaitForSemaphore(semNetworkConnected, OS_TIME_OUT_WAIT_FOREVER);
    OS_Sleep(3000);
    Trace(2, "Collecting Sensor Data");

    // Get the temperature from the OneWire temperature sensor
    OneWire oneWire;
    oneWire.begin(I2C2);
    DallasTemperature oneWireSensors(&oneWire);
    oneWireSensors.begin();
    oneWireSensors.requestTemperatures();
    Trace(1, "oneWire device count: %d", oneWireSensors.getDeviceCount());
    OS_Sleep(1);
    float tempC = oneWireSensors.getTempCByIndex(0);
    if (tempC == -127)
    {
        OS_Sleep(1);
        tempC = oneWireSensors.getTempCByIndex(0);
    }
    Trace(2, "oneWire temperature sensor: %0.2f%cC", tempC, (char)0xB0);
    OS_Sleep(2000);

    // Get battery voltage
    uint8_t percent;
    uint16_t v = PM_Voltage(&percent);
    Trace(2, "Battery voltage: %04dmV", v);

    // Get the time from GSM network
    RTC_Time_t time;
    TIME_GetRtcTime(&time);
    char timeString[25];
    snprintf(
        timeString, sizeof(timeString),
        "%02d:%02d:%02d",
        (time.hour + time.timeZone) % 24,
        time.minute,
        time.second
    );

    char tempString[25];
    snprintf(
        tempString, sizeof(tempString),
        " %0.2f*C",
        tempC
        );
    char batteryString[10];
    snprintf(
        batteryString, sizeof(batteryString),
        " %04dmV",
        v);

    // Finished with I2C
    I2C_Close(I2C2);

    // Update OLED
    i2c_init();
    OLED_Init();
    OLED_CLS();
    OLED_ShowStr(0, 1, tempString, 2);
    OLED_ShowStr(75, 0, timeString, 1);
    OLED_ShowStr(80, 1, batteryString, 1);

    // Convert GPS unit ddmm.mmmm to degree(°)
    int temp = (int)(gpsInfo->rmc.latitude.value/gpsInfo->rmc.latitude.scale/100);
    double latitude = temp+(double)(gpsInfo->rmc.latitude.value - temp*gpsInfo->rmc.latitude.scale*100)/gpsInfo->rmc.latitude.scale/60.0;
    temp = (int)(gpsInfo->rmc.longitude.value/gpsInfo->rmc.longitude.scale/100);
    double longitude = temp+(double)(gpsInfo->rmc.longitude.value - temp*gpsInfo->rmc.longitude.scale*100)/gpsInfo->rmc.longitude.scale/60.0;

    uint8_t isFixed = gpsInfo->gsa[0].fix_type > gpsInfo->gsa[1].fix_type ? gpsInfo->gsa[0].fix_type : gpsInfo->gsa[1].fix_type;
    char *isFixedStr = "no fix";
    if (isFixed == 2)
        isFixedStr = "2D fix";
    else if (isFixed == 3)
    {
        if (gpsInfo->gga.fix_quality == 1)
            isFixedStr = "3D fix";
        else if (gpsInfo->gga.fix_quality == 2)
            isFixedStr = "3D/DGPS fix";
    }

    snprintf(
        postData, postDataLen,
        "\r\nid=1&waterTemp=%0.2f&BusVoltage=%1.4f&lat=%0.4f&lng=%0.4f&alt=%0.2f&sats=%d&fix=%s",
        tempC,
        (float)v/1000,
        latitude,
        longitude,
        gpsInfo->gga.altitude,
        gpsInfo->gga.satellites_tracked,
        isFixedStr);

    GPIO_Set(GPIO_PIN28, GPIO_LEVEL_HIGH);
    if (Https_Post(
            "oceanswims.nz",
            "443",
            "/track/a9",
            postData) < 0)
    {
        Trace(1, "https post fail");
    }
    else
    {
        Trace(1, "https post success");
    }

    Trace(1, "Data send complete, shutting down");
    // Turn off blue led
    GPIO_Set(GPIO_PIN27, GPIO_LEVEL_LOW);
    GPIO_Set(GPIO_PIN28, GPIO_LEVEL_LOW);

    // Set 'DONE' pin high
    OS_Sleep(1000);
    PM_PowerEnable(POWER_TYPE_LCD, true);
    GPIO_Set(GPIO_PIN14, GPIO_LEVEL_HIGH);
    OS_Sleep(1000 * 10);

    OS_Sleep(1000 * 100);

}

void MainTask(void *pData)
{
    API_Event_t *event = NULL;
    semNetworkConnected = OS_CreateSemaphore(0);

    // This task gets data from the various sensors then makes an HTTPS request
    // to the server to get them onto the internet
    secondTaskHandle =
        OS_CreateTask(SecondTask, NULL, NULL, SECOND_TASK_STACK_SIZE,
                      SECOND_TASK_PRIORITY, 0, 0, SECOND_TASK_NAME);

    // Timeout task will automatically abort this run after a set time to avoid
    // draining the battery is there's a recuring fault, eg mobile or internet
    // connectivity issue
    timeoutTaskHandle =
        OS_CreateTask(TimeoutTask, NULL, NULL, TIMEOUT_TASK_STACK_SIZE,
                      TIMEOUT_TASK_PRIORITY, 0, 0, TIMEOUT_TASK_NAME);

    while (1)
    {
        if (OS_WaitEvent(mainTaskHandle, (void **)&event,
                         OS_TIME_OUT_WAIT_FOREVER))
        {
            EventDispatch(event);
            OS_Free(event->pParam1);
            OS_Free(event->pParam2);
            OS_Free(event);
        }
    }
}

extern "C"
{

    void buoybasic_Main(void)
    {
        mainTaskHandle = OS_CreateTask(MainTask, NULL, NULL, MAIN_TASK_STACK_SIZE,
                                       MAIN_TASK_PRIORITY, 0, 0, MAIN_TASK_NAME);
        OS_SetUserMainHandle(&mainTaskHandle);
    }
}
