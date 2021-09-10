#include "railguard/utils/event_sender.h"

#include <railguard/utils/maps.h>

#include <stdlib.h>

// --=== Types ===--

typedef struct rg_event_sender
{
    rg_hash_map *handlers;
} rg_event_sender;

// --=== Event Sender ===--

rg_event_sender *rg_create_event_sender(void)
{
    // Allocate sender
    rg_event_sender *sender = calloc(1, sizeof(rg_event_sender));
    if (sender == NULL) {
        return NULL;
    }

    // Create handlers hash map
    sender->handlers = rg_create_hash_map();
    if (sender->handlers == NULL) {
        free(sender);
        return NULL;
    }

    return sender;
}

void rg_destroy_event_sender(rg_event_sender **event_sender)
{
    // Destroy the handlers map
    rg_destroy_hash_map(&(*event_sender)->handlers);

    // Free the sender itself
    free(*event_sender);
    *event_sender = NULL;
}

bool rg_event_sender_register_listener(rg_event_sender *event_sender, const char* handler_name, rg_event_handler handler)
{
    // Check if the name is already taken
    if (rg_hash_map_get(event_sender->handlers, handler_name) != NULL) {
        // In that case, we refuse to add the listener.
        return false;
    }

    // Otherwise, add it to the map
    return rg_hash_map_set(event_sender->handlers, handler_name, handler);
}

void rg_event_sender_unregister_listener(rg_event_sender *event_sender, const char *handler_name)
{
    // Simply erase the name from the map
    rg_hash_map_erase(event_sender->handlers, handler_name);
}


void rg_event_sender_send_event(rg_event_sender *event_sender, void *data)
{
    rg_hash_map_it it = rg_hash_map_iterator(event_sender->handlers);
    // Call the handlers with the data
    while (rg_hash_map_next(&it)) {
        ((rg_event_handler) it.value)(data);
    }
}
