#pragma once

#include <stdbool.h>
#include <stdint.h>

// --=== Constants ===--

#define RG_EVENT_HANDLER_NULL_ID 0

// --=== Types ===--

typedef void (*rg_event_handler_function)(void *event_data, void *user_data);

typedef uint32_t rg_event_handler_id;

typedef struct rg_event_handler
{
    rg_event_handler_function pfn_handler;
    void                     *user_data;
} rg_event_handler;

typedef struct rg_event_sender rg_event_sender;

// --=== Event Sender ===--

rg_event_sender    *rg_create_event_sender(void);
void                rg_destroy_event_sender(rg_event_sender **event_sender);
rg_event_handler_id rg_event_sender_register_listener(rg_event_sender *event_sender, rg_event_handler handler);
void                rg_event_sender_unregister_listener(rg_event_sender *event_sender, rg_event_handler_id handler_id);
void                rg_event_sender_send_event(rg_event_sender *event_sender, void *data);