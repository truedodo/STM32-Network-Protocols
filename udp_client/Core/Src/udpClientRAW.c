/***************************************************************************************************************
 * File:        udpClientRAW.c
 * Author:      ControllersTech.com
 * Updated:     Oct 11, 2025 (with multiple PWM JSON parsing)
 ***************************************************************************************************************/

#include "lwip/pbuf.h"
#include "lwip/udp.h"
#include "lwip/tcp.h"

#include <string.h>
#include <stdio.h>

#include "udpClientRAW.h"
#include "cJSON.h"

// --- ADDED ---
// Global volatile variables to store the PWM values received via UDP
volatile int32_t g_pwm_value1 = 0;
volatile int32_t g_pwm_value2 = 0;
volatile int32_t g_pwm_value3 = 0;
volatile int32_t g_pwm_value4 = 0;
// -------------

int a = 0;

void udp_receive_callback(void *arg, struct udp_pcb *upcb, struct pbuf *p, const ip_addr_t *addr, u16_t port);
static void udpClient_send(void);

struct udp_pcb *upcb;
int counter = 0;

extern TIM_HandleTypeDef htim1;

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    udpClient_send();
}

void udpClient_connect(void)
{
    err_t err;

    upcb = udp_new();

    ip_addr_t myIPaddr;
    IP_ADDR4(&myIPaddr, 192, 168, 1, 247);
    udp_bind(upcb, &myIPaddr, 7);

    ip_addr_t DestIPaddr;
//    IP_ADDR4(&DestIPaddr, 192, 168, 1, 7);
//    err = udp_connect(upcb, &DestIPaddr, 58777);

    if (err == ERR_OK)
    {
        udpClient_send();
        udp_recv(upcb, udp_receive_callback, NULL);
    }
}

static void udpClient_send(void)
{
    struct pbuf *txBuf;
    const char *data = "{\"PWM\": 500}";
    int len = strlen(data);

    txBuf = pbuf_alloc(PBUF_TRANSPORT, len, PBUF_RAM);
    if (txBuf != NULL)
    {
        pbuf_take(txBuf, data, len);
        udp_send(upcb, txBuf);
        pbuf_free(txBuf);
    }
}

/**
 * @brief Callback when a UDP packet is received.
 * Expects JSON like: {"PWM1":1000, "PWM2":1200, "PWM3":800, "PWM4":900}
 */
void udp_receive_callback(void *arg, struct udp_pcb *upcb, struct pbuf *p,
                          const ip_addr_t *addr, u16_t port)
{
    if (p == NULL)
        return;

    // 1. Copy payload and ensure null termination
    char json_string[p->tot_len + 1];
    pbuf_copy_partial(p, json_string, p->tot_len, 0);
    json_string[p->tot_len] = '\0';

    // 2. Parse JSON
    cJSON *root = cJSON_Parse((const char *)json_string);
    if (root == NULL)
    {
        printf("Error parsing JSON: %s\r\n", cJSON_GetErrorPtr());
        pbuf_free(p);
        return;
    }

    a = 1; // just a flag that data was received

    // 3. Extract each PWM value
    cJSON *pwm_item1 = cJSON_GetObjectItemCaseSensitive(root, "PWM1");
    cJSON *pwm_item2 = cJSON_GetObjectItemCaseSensitive(root, "PWM2");
    cJSON *pwm_item3 = cJSON_GetObjectItemCaseSensitive(root, "PWM3");
    cJSON *pwm_item4 = cJSON_GetObjectItemCaseSensitive(root, "PWM4");

    if (cJSON_IsNumber(pwm_item1))
    {
        g_pwm_value1 = (uint32_t)pwm_item1->valueint;
        printf("Received PWM1: %lu\r\n", g_pwm_value1);
    }

    if (cJSON_IsNumber(pwm_item2))
    {
        g_pwm_value2 = (uint32_t)pwm_item2->valueint;
        printf("Received PWM2: %lu\r\n", g_pwm_value2);
    }

    if (cJSON_IsNumber(pwm_item3))
    {
        g_pwm_value3 = (uint32_t)pwm_item3->valueint;
        printf("Received PWM3: %lu\r\n", g_pwm_value3);
    }

    if (cJSON_IsNumber(pwm_item4))
    {
        g_pwm_value4 = (uint32_t)pwm_item4->valueint;
        printf("Received PWM4: %lu\r\n", g_pwm_value4);
    }

    // 4. Clean up
    cJSON_Delete(root);
    pbuf_free(p);
    counter++;
}
