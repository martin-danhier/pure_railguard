#pragma once

// --=== Types ===--

typedef void (*rg_event_handler)(void *data);
typedef struct rg_event_sender rg_event_sender;

// --=== Event Sender ===--

rg_event_sender *rg_create_event_sender(void);
void             rg_event_sender_register_listener(rg_event_sender *event_sender, rg_event_handler handler);
void rg_destroy_event_sender(rg_event_sender **event_sender);