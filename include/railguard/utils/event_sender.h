#pragma once

#include <stdbool.h>

// --=== Types ===--

typedef struct rg_event_handler {
    void (*pfn_handler)(void *event_data, void *user_data);
    void *user_data;
} rg_event_handler;

typedef struct rg_event_sender rg_event_sender;

// --=== Event Sender ===--

rg_event_sender *rg_create_event_sender(void);
void             rg_destroy_event_sender(rg_event_sender **event_sender);
bool             rg_event_sender_register_listener(rg_event_sender *event_sender, const char *handler_name, rg_event_handler handler);
void             rg_event_sender_unregister_listener(rg_event_sender *event_sender, const char *handler_name);
void             rg_event_sender_send_event(rg_event_sender *event_sender, void *data);