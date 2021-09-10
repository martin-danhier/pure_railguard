#include "railguard/utils/event_sender.h"

#include <stdlib.h>

// --=== Types ===--

typedef struct rg_event_sender
{

} rg_event_sender;

// --=== Event Sender ===--

rg_event_sender *rg_create_event_sender(void)
{
    rg_event_sender *sender = calloc(1, sizeof(rg_event_sender));

    return sender;
}

void rg_destroy_event_sender(rg_event_sender **event_sender)
{
    free(*event_sender);
    *event_sender = NULL;
}

void rg_event_sender_register_listener(rg_event_sender *event_sender, rg_event_handler handler)
{

}
