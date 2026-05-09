/*
 * === DO NOT MODIFY THIS FILE ===
 * During testing, we will replace this file with our
 * own. You have been warned.
 * === DO NOT MODIFY THIS FILE ===
 */

#ifndef FZL_EVENTS_H
#define FZL_EVENTS_H

typedef enum fzl_event {
    FZL_INIT            = 0x00,
    FZL_SEND_INPUT      = 0x01,
    FZL_RNR_INIT        = 0x02,
    FZL_RNR_RECV_INPUT  = 0x03,
    FZL_RNR_LAUNCH      = 0x04,
    FZL_RNR_SEND_STATUS = 0x05,
    FZL_RNR_FINI        = 0x06,
    FZL_RECV_STATUS     = 0x07,
    FZL_FINI            = 0xFF,
    FZL_NONE
} FZL_EVENT;

#endif