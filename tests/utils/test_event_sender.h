#pragma once

#include "../framework/test_framework.h"
#include <railguard/utils/event_sender.h>

typedef struct rg_test_event_sender_user_data
{
    uint64_t listener1_received_data;
    uint64_t listener2_received_data;
    uint64_t listener3_received_data;
} rg_test_event_sender_user_data;

// Define callbacks

void rg_test_event_sender_callback1(const uint64_t *event_data, rg_test_event_sender_user_data *user_data)
{
    user_data->listener1_received_data = *event_data;
}

void rg_test_event_sender_callback2(const uint64_t *event_data, rg_test_event_sender_user_data *user_data)
{
    // Add a value to it so that we can know which callback was called
    user_data->listener2_received_data = *event_data + 42;
}

void rg_test_event_sender_callback3(const uint64_t *event_data, rg_test_event_sender_user_data *user_data)
{
    // Add a value to it so that we can know which callback was called
    user_data->listener3_received_data = *event_data + 789;
}

TEST(EventSender)
{
    // Create a new event sender.
    rg_event_sender *event_sender = rg_create_event_sender();
    ASSERT_NOT_NULL(event_sender);

    // Create a struct so that we can see if the listeners were called.
    rg_test_event_sender_user_data user_data = {};

    // Register listeners.
    rg_event_handler_id handler1 =
        rg_event_sender_register_listener(event_sender,
                                          (rg_event_handler) {
                                              .pfn_handler = (rg_event_handler_function) rg_test_event_sender_callback1,
                                              .user_data   = &user_data,
                                          });
    EXPECT_TRUE(handler1 != RG_EVENT_HANDLER_NULL_ID);

    rg_event_handler_id handler2 =
        rg_event_sender_register_listener(event_sender,
                                          (rg_event_handler) {
                                              .pfn_handler = (rg_event_handler_function) rg_test_event_sender_callback2,
                                              .user_data   = &user_data,
                                          });
    EXPECT_TRUE(handler2 != RG_EVENT_HANDLER_NULL_ID);

    rg_event_handler_id handler3 =
        rg_event_sender_register_listener(event_sender,
                                          (rg_event_handler) {
                                              .pfn_handler = (rg_event_handler_function) rg_test_event_sender_callback3,
                                              .user_data   = &user_data,
                                          });
    EXPECT_TRUE(handler3 != RG_EVENT_HANDLER_NULL_ID);

    // Send an event.
    uint64_t event_data = 42;
    rg_event_sender_send_event(event_sender, &event_data);

    // Check that the listeners were called.
    EXPECT_TRUE(user_data.listener1_received_data == event_data);
    EXPECT_TRUE(user_data.listener2_received_data == event_data + 42);
    EXPECT_TRUE(user_data.listener3_received_data == event_data + 789);

    // Reset user data.
    user_data.listener1_received_data = 0;
    user_data.listener2_received_data = 0;
    user_data.listener3_received_data = 0;

    // Try to remove one of the listeners.
    rg_event_sender_unregister_listener(event_sender, handler2);

    // Send an event.
    event_data = 84;
    rg_event_sender_send_event(event_sender, &event_data);

    // Check that the listeners were called, but not the one that was removed.
    EXPECT_TRUE(user_data.listener1_received_data == event_data);
    EXPECT_TRUE(user_data.listener2_received_data == 0);
    EXPECT_TRUE(user_data.listener3_received_data == event_data + 789);

    // Clean up event sender.
    rg_destroy_event_sender(&event_sender);
    ASSERT_NULL(event_sender);
}