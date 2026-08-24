/****************************************************************************
 * apps/examples/fitness/http_client.h
 *
 * HTTP client module for communicating with the fitness backend API.
 ****************************************************************************/

#ifndef __HTTP_CLIENT_H
#define __HTTP_CLIENT_H

#include <stdint.h>
#include <stdbool.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define HTTP_MAX_RESPONSE   4096
#define HTTP_MAX_URL        256

/****************************************************************************
 * Public Type Definitions
 ****************************************************************************/

typedef struct
{
  int  status_code;
  char body[HTTP_MAX_RESPONSE];
  int  body_len;
} http_response_t;

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

void http_client_init(void);
int http_get(const char *path, const char *token, http_response_t *resp);
int http_post(const char *path, const char *token,
              const char *json, http_response_t *resp);
int http_upload_fitness_record(const char *device_sn, const char *json);
int http_upload_heartbeat(const char *device_sn, const char *json);

#endif /* __HTTP_CLIENT_H */
