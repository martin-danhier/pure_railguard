#include "railguard/utils/event_sender.h"

#include <railguard/utils/arrays.h>
#include <railguard/utils/memory.h>
#include <railguard/utils/storage.h>

#include <stdlib.h>

// --=== Types ===--

 typedef struct rg_event_sender
 {
     rg_storage *handlers;
 } rg_event_sender;

 // --=== Event Sender ===--

 rg_event_sender *rg_create_event_sender(void)
 {
     // Allocate sender
     rg_event_sender *sender = rg_calloc(1, sizeof(rg_event_sender));
     if (sender == NULL) {
         return NULL;
     }

     // Create handlers_lookup_map hash map
     sender->handlers = rg_create_storage(sizeof(rg_event_handler));
     if (sender->handlers == NULL) {
         rg_free(sender);
         return NULL;
     }

     return sender;
 }

 void rg_destroy_event_sender(rg_event_sender **event_sender)
 {
     if (event_sender == NULL || *event_sender == NULL) {
         return;
     }

     // Destroy the handlers storage
     rg_destroy_storage(&(*event_sender)->handlers);

     // Free the sender itself
     rg_free(*event_sender);
     *event_sender = NULL;
 }

 rg_event_handler_id rg_event_sender_register_listener(rg_event_sender *event_sender, rg_event_handler handler)
 {
     if (event_sender == NULL) {
         return RG_EVENT_HANDLER_NULL_ID;
     }

     // Save handler
     return rg_storage_push(event_sender->handlers, &handler);
 }

 void rg_event_sender_unregister_listener(rg_event_sender *event_sender, rg_event_handler_id handler_name)
 {
     if (event_sender == NULL) {
         return;
     }

     // Simply erase the name from the storage
     rg_storage_erase(event_sender->handlers, handler_name);
 }

 void rg_event_sender_send_event(rg_event_sender *event_sender, void *data)
 {
     if (event_sender == NULL) {
         return;
     }

     rg_storage_it it = rg_storage_iterator(event_sender->handlers);
     // Call the handlers_lookup_map with the data
     while (rg_storage_next(&it)) {
         rg_event_handler *handler = it.value;
         handler->pfn_handler(data, handler->user_data);
     }
 }
