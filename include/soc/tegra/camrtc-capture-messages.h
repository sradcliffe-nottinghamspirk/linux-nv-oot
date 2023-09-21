/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2016-2023, NVIDIA CORPORATION & AFFILIATES.  All rights reserved.
 */

/**
 * @file camrtc-capture-messages.h
 *
 * @brief Capture control and Capture IVC messages
 */

#ifndef INCLUDE_CAMRTC_CAPTURE_MESSAGES_H
#define INCLUDE_CAMRTC_CAPTURE_MESSAGES_H

#include "camrtc-capture.h"

#pragma GCC diagnostic error "-Wpadded"

/**
 * @brief Standard message header for all capture and capture-control IVC messages.
 *
 * Control Requests not associated with a specific channel
 * will use an opaque transaction ID rather than channel_id.
 * The transaction ID in the response message is copied from
 * the request message.
 */
struct CAPTURE_MSG_HEADER {
	/** Message identifier (See @ref CapCtrlMsgType). */
	uint32_t msg_id;
	/** @anon_union */
	union {
		/**
		 * @anon_union_member
		 * Capture channel ID [0, UINT32_MAX]. Used when the message
		 * is related to a specific capture channel. The channel ID
		 * must be a valid ID previously assigned by RCE FW.
		 */
		uint32_t channel_id;
		/**
		 * @anon_union_member
		 * Transaction id [0, UINT32_MAX]. Used when the message is not
		 * related to a specific capture channel. The transaction ID in
		 * a request message must be unique and may not collide with any
		 * existing channel ID. The transaction in a response message
		 * must match the transaction ID of the associated request.
		 */
		uint32_t transaction;
	};
} CAPTURE_IVC_ALIGN;

/**
 * @defgroup CapCtrlMsgType Message types for capture-control IVC channel messages
 *
 * The capture-control IVC channel is established during boot using the
 * @ref CAMRTC_HSP_CH_SETUP command. The IVC channel is bi-directional
 * and can be used to carry out control operations on RCE firmware. Each
 * control operation consists of a request message and an associated
 * response message.
 *
 * The overall message structure is defined by @ref CAPTURE_CONTROL_MSG
 * and the structure is the same for both requests and responses. Each control
 * message consists of a common @ref CAPTURE_MSG_HEADER followed by message
 * specific data.
 *
 * The type of a message is identified by @ref CAPTURE_MSG_HEADER::msg_id.
 *
 * Requests and responses may be associated with a logical channel. All
 * requests on the same logical channel must be strictly serialized, so
 * that only a single request may be pending at any time. Requests and
 * responses must specify the logical channel ID in the
 * @ref CAPTURE_MSG_HEADER::channel_id. The channel ID is assigned by
 * RCE FW when the logical channel is created.
 *
 * Requests and responses that are not associated with a specific logical
 * channel must instead specify a transaction ID in
 * @ref CAPTURE_MSG_HEADER::transaction. The transaction ID must be unique
 * for each request and must not collide with any existing channel ID.
 * A response message must use the same transaction ID as the associated
 * request message. Examples of control messages that are not associated
 * with an existing logical channel are messages that are used to
 * establish a new logical channel. There are also other control operations
 * that are not specific to a logical channel.
 *
 * @{
 */

/**
 * @defgroup ViCapCtrlMsgType Message types for VI capture channel control messages
 *
 * Capture channel control messages are used to set up, reset, and release
 * channels for capturing images from an imaging stream source, as well as
 * execute other control operations on the VI capture channel.
 *
 * See @ref CapCtrlMsgType for more information on the message protocol.
 *
 * @{
 */

/**
 * @brief VI capture channel setup request.
 *
 * This is a @ref CapCtrlMsgType "capture control message" to
 * allocate a VI capture channel and associated resources.
 *
 * @pre The capture-control IVC channel has been set up during
 *      boot using the @ref CAMRTC_HSP_CH_SETUP command.
 *
 * @par Header
 * - @ref CAPTURE_CONTROL_MSG@b::@ref CAPTURE_MSG_HEADER "header"
 *   - @ref CAPTURE_MSG_HEADER::msg_id "msg_id" = @ref CAPTURE_CHANNEL_SETUP_REQ
 *   - @ref CAPTURE_MSG_HEADER::transaction "transaction" = <em>unique ID</em>
 *
 * @par Payload
 * - @ref CAPTURE_CHANNEL_SETUP_REQ_MSG
 *
 * @par Response
 * - @ref CAPTURE_CHANNEL_SETUP_RESP
 */
#define CAPTURE_CHANNEL_SETUP_REQ		MK_U32(0x1E)

/**
 * @brief VI capture channel setup response.
 *
 * This is a @ref CapCtrlMsgType "capture control message" received in
 * response to a @ref CAPTURE_CHANNEL_SETUP_REQ message.
 *
 * @pre A @ref CAPTURE_CHANNEL_SETUP_REQ message has been sent.
 *
 * @par Header
 * - @ref CAPTURE_CONTROL_MSG@b::@ref CAPTURE_MSG_HEADER "header"
 *   - @ref CAPTURE_MSG_HEADER::msg_id "msg_id" = @ref CAPTURE_CHANNEL_SETUP_RESP
 *   - @ref CAPTURE_MSG_HEADER::transaction "transaction" =
 *     @ref CAPTURE_CHANNEL_SETUP_REQ_MSG@b::@ref CAPTURE_MSG_HEADER "header"@b::@ref CAPTURE_MSG_HEADER::transaction "transaction"
 *
 * @par Payload
 *  - @ref CAPTURE_CHANNEL_SETUP_RESP_MSG
 */
#define CAPTURE_CHANNEL_SETUP_RESP		MK_U32(0x11)

/**
 * @brief VI capture channel reset request.
 *
 * This is a @ref CapCtrlMsgType "capture control message" to
 * reset a VI capture channel. The client must also send a @ref
 * CAPTURE_RESET_BARRIER_IND message on the @em capture IVC channel
 * in order to define a boundary between capture requests submitted
 * before the reset and requests submitted after it.
 *
 * When RCE FW receives the @ref CAPTURE_CHANNEL_RESET_REQ message, it
 * will cancel all capture requests in the channel queue upto the @ref
 * CAPTURE_RESET_BARRIER_IND message. The response is sent after the
 * RCE side channel cleanup is complete. If the reset barrier is not
 * received within 5 ms, all requests currently in the queue will be
 * cleared and a @ref CAPTURE_ERROR_TIMEOUT error will be reported
 * in the response message.
 *
 * @pre A VI capture channel has been set up with
 *      @ref CAPTURE_CHANNEL_SETUP_REQ.
 *
 * @par Header
 * - @ref CAPTURE_CONTROL_MSG@b::@ref CAPTURE_MSG_HEADER "header"
 *   - @ref CAPTURE_MSG_HEADER::msg_id "msg_id" = @ref CAPTURE_CHANNEL_RESET_REQ
 *   - @ref CAPTURE_MSG_HEADER::channel_id "channel_id" =
 *     @ref CAPTURE_CHANNEL_SETUP_RESP_MSG@b::@ref CAPTURE_MSG_HEADER "header"@b::@ref CAPTURE_MSG_HEADER::channel_id "channel_id"
 *
 * @par Payload
 * - @ref CAPTURE_CHANNEL_RESET_REQ_MSG
 *
 * @par Response
 * - @ref CAPTURE_CHANNEL_RESET_RESP
 */
#define CAPTURE_CHANNEL_RESET_REQ		MK_U32(0x12)

/**
 * @brief VI capture channel reset response.
 *
 * This is a @ref CapCtrlMsgType "capture control message" received in
 * response to a @ref CAPTURE_CHANNEL_RESET_REQ message.
 *
 * @pre A @ref CAPTURE_CHANNEL_RESET_REQ message has been sent to the
 *      logical channel.
 *
 * @par Header
 * - @ref CAPTURE_CONTROL_MSG@b::@ref CAPTURE_MSG_HEADER "header"
 *   - @ref CAPTURE_MSG_HEADER::msg_id "msg_id" = @ref CAPTURE_CHANNEL_RESET_RESP
 *   - @ref CAPTURE_MSG_HEADER::channel_id "channel_id" =
 *     @ref CAPTURE_CHANNEL_RESET_REQ_MSG@b::@ref CAPTURE_MSG_HEADER "header"@b::@ref CAPTURE_MSG_HEADER::channel_id "channel_id"
 *
 * @par Payload
 * - @ref CAPTURE_CHANNEL_RESET_RESP_MSG
 */
#define CAPTURE_CHANNEL_RESET_RESP		MK_U32(0x13)

/**
 * @brief VI capture channel release request.
 *
 * This is a @ref CapCtrlMsgType "capture control message" to
 * release a VI capture channel. Cancels all pending capture
 * requests.
 *
 * @pre A VI capture channel has been set up with
 *     @ref CAPTURE_CHANNEL_SETUP_REQ.
 *
 * @par Header
 * - @ref CAPTURE_CONTROL_MSG@b::@ref CAPTURE_MSG_HEADER "header"
 *   - @ref CAPTURE_MSG_HEADER::msg_id "msg_id" = @ref CAPTURE_CHANNEL_RELEASE_REQ
 *   - @ref CAPTURE_MSG_HEADER::channel_id "channel_id" =
 *     @ref CAPTURE_CHANNEL_SETUP_RESP_MSG@b::@ref CAPTURE_MSG_HEADER "header"@b::@ref CAPTURE_MSG_HEADER::channel_id "channel_id"
 *
 * @par Payload
 * - @ref CAPTURE_CHANNEL_RELEASE_REQ_MSG
 *
 * @par Response
 * - @ref CAPTURE_CHANNEL_RELEASE_RESP
 */
#define CAPTURE_CHANNEL_RELEASE_REQ		MK_U32(0x14)

/**
 * @brief VI capture channel release response.
 *
 * This is a @ref CapCtrlMsgType "capture control message" received in
 * response to a @ref CAPTURE_CHANNEL_RELEASE_REQ message.
 *
 * @pre A @ref CAPTURE_CHANNEL_RELEASE_REQ message has been sent to the
 *      logical channel.
 *
 * @par Header
 * - @ref CAPTURE_CONTROL_MSG@b::@ref CAPTURE_MSG_HEADER "header"
 *   - @ref CAPTURE_MSG_HEADER::msg_id "msg_id" = @ref CAPTURE_CHANNEL_RELEASE_RESP
 *   - @ref CAPTURE_MSG_HEADER::channel_id "channel_id" =
 *     @ref CAPTURE_CHANNEL_RELEASE_REQ_MSG@b::@ref CAPTURE_MSG_HEADER "header"@b::@ref CAPTURE_MSG_HEADER::channel_id "channel_id"
 *
 * @par Payload
 * - @ref CAPTURE_CHANNEL_RELEASE_RESP_MSG
 */
#define CAPTURE_CHANNEL_RELEASE_RESP		MK_U32(0x15)
/** @} */

/**
 * @defgroup ViCapCtrlMsgTypeNonsafe Message types for VI capture channel control messages (non-safety)
 * @{
 */
#define CAPTURE_COMPAND_CONFIG_REQ		MK_U32(0x16)
#define CAPTURE_COMPAND_CONFIG_RESP		MK_U32(0x17)
#define CAPTURE_PDAF_CONFIG_REQ			MK_U32(0x18)
#define CAPTURE_PDAF_CONFIG_RESP		MK_U32(0x19)
#define CAPTURE_SYNCGEN_ENABLE_REQ		MK_U32(0x1A)
#define CAPTURE_SYNCGEN_ENABLE_RESP		MK_U32(0x1B)
#define CAPTURE_SYNCGEN_DISABLE_REQ		MK_U32(0x1C)
#define CAPTURE_SYNCGEN_DISABLE_RESP		MK_U32(0x1D)
/** @} */

/**
 * @defgroup IspCapCtrlMsgType Message types for ISP capture-control IVC channel messages.
 * @{
 */

/**
 * @brief ISP capture channel setup request.
 *
 * This is a @ref CapCtrlMsgType "capture control message" to
 * allocate an ISP capture channel and associated resources.
 *
 * @pre The capture-control IVC channel has been set up during
 *      boot using the @ref CAMRTC_HSP_CH_SETUP command.
 *
 * @par Header
 * - @ref CAPTURE_CONTROL_MSG@b::@ref CAPTURE_MSG_HEADER "header"
 *   - @ref CAPTURE_MSG_HEADER::msg_id "msg_id" = @ref CAPTURE_CHANNEL_ISP_SETUP_REQ
 *   - @ref CAPTURE_MSG_HEADER::transaction "transaction" = <em>unique ID</em>
 *
 * @par Payload
 * - @ref CAPTURE_CHANNEL_ISP_SETUP_REQ_MSG
 *
 * @par Response
 * - @ref CAPTURE_CHANNEL_ISP_SETUP_RESP
 */
#define CAPTURE_CHANNEL_ISP_SETUP_REQ		MK_U32(0x20)

/**
 * @brief ISP capture channel setup response.
 *
 * This is a @ref CapCtrlMsgType "capture control message" received in
 * response to a @ref CAPTURE_CHANNEL_ISP_SETUP_REQ message.
 *
 * @pre A @ref CAPTURE_CHANNEL_ISP_SETUP_REQ message has been sent.
 *
 * @par Header
 * - @ref CAPTURE_CONTROL_MSG@b::@ref CAPTURE_MSG_HEADER "header"
 *   - @ref CAPTURE_MSG_HEADER::msg_id "msg_id" = @ref CAPTURE_CHANNEL_ISP_SETUP_RESP
 *   - @ref CAPTURE_MSG_HEADER::transaction "transaction" =
 *     @ref CAPTURE_CHANNEL_ISP_SETUP_REQ_MSG@b::@ref CAPTURE_MSG_HEADER "header"@b::@ref CAPTURE_MSG_HEADER::transaction "transaction"
 *
 * @par Payload
 * - @ref CAPTURE_CHANNEL_SETUP_ISP_RESP_MSG
 */
#define CAPTURE_CHANNEL_ISP_SETUP_RESP		MK_U32(0x21)

/**
 * @brief ISP capture channel reset request.
 *
 * This is a @ref CapCtrlMsgType "capture control message" to
 * reset an ISP capture channel. The client must also send a @ref
 * CAPTURE_ISP_RESET_BARRIER_IND message on the @em capture IVC channel
 * in order to define a boundary between capture requests submitted
 * before the reset and requests submitted after it.
 *
 * When RCE FW receives the @ref CAPTURE_CHANNEL_ISP_RESET_REQ message,
 * it will cancel all requests in the channel queue upto the @ref
 * CAPTURE_ISP_RESET_BARRIER_IND message. The response is sent after the
 * RCE side channel cleanup is complete. If the reset barrier is not
 * received within 5 ms, all requests currently in the queue will be
 * cleared and a @ref CAPTURE_ERROR_TIMEOUT error will be reported
 * in the response message.
 *
 * @pre An ISP capture channel has been set up with
 *      @ref CAPTURE_CHANNEL_ISP_SETUP_REQ.
 *
 * @par Header
 * - @ref CAPTURE_CONTROL_MSG@b::@ref CAPTURE_MSG_HEADER "header"
 *   - @ref CAPTURE_MSG_HEADER::msg_id "msg_id" = @ref CAPTURE_CHANNEL_ISP_RESET_REQ
 *   - @ref CAPTURE_MSG_HEADER::channel_id "channel_id" =
 *     @ref CAPTURE_CHANNEL_ISP_SETUP_RESP_MSG@b::@ref CAPTURE_MSG_HEADER "header"@b::@ref CAPTURE_MSG_HEADER::channel_id "channel_id"
 *
 * @par Payload
 * - @ref CAPTURE_CHANNEL_ISP_RESET_REQ_MSG
 *
 * @par Response
 * - @ref CAPTURE_CHANNEL_ISP_RESET_RESP
 */
#define CAPTURE_CHANNEL_ISP_RESET_REQ		MK_U32(0x22)

/**
 * @brief ISP capture channel reset response.
 *
 * This is a @ref CapCtrlMsgType "capture control message" received in
 * response to a @ref CAPTURE_CHANNEL_ISP_RESET_REQ message.
 *
 * @pre A @ref CAPTURE_CHANNEL_ISP_RESET_REQ message has been sent
 *      to the logical channel.
 *
 * @par Header
 * - @ref CAPTURE_CONTROL_MSG@b::@ref CAPTURE_MSG_HEADER "header"
 *   - @ref CAPTURE_MSG_HEADER::msg_id "msg_id" = @ref CAPTURE_CHANNEL_ISP_RESET_RESP
 *   - @ref CAPTURE_MSG_HEADER::channel_id "channel_id" =
 *     @ref CAPTURE_CHANNEL_ISP_RESET_REQ_MSG@b::@ref CAPTURE_MSG_HEADER "header"@b::@ref CAPTURE_MSG_HEADER::channel_id "channel_id"
 *
 * @par Payload
 *   - @ref CAPTURE_CHANNEL_ISP_RESET_RESP_MSG
 */
#define CAPTURE_CHANNEL_ISP_RESET_RESP		MK_U32(0x23)

/**
 * @brief ISP capture channel release request.
 *
 * This is a @ref CapCtrlMsgType "capture control message" to
 * release an ISP capture channel. Cancels all pending requests.
 *
 * @pre An ISP capture channel has been set up with
 *     @ref CAPTURE_CHANNEL_ISP_SETUP_REQ.
 *
 * @par Header
 * - @ref CAPTURE_CONTROL_MSG@b::@ref CAPTURE_MSG_HEADER "header"
 *   - @ref CAPTURE_MSG_HEADER::msg_id "msg_id" = @ref CAPTURE_CHANNEL_ISP_RELEASE_REQ
 *   - @ref CAPTURE_MSG_HEADER::channel_id "channel_id" =
 *     @ref CAPTURE_CHANNEL_ISP_SETUP_RESP_MSG@b::@ref CAPTURE_MSG_HEADER "header"@b::@ref CAPTURE_MSG_HEADER::channel_id "channel_id"
 *
 * @par Payload
 * - @ref CAPTURE_CHANNEL_ISP_RELEASE_REQ_MSG
 *
 * @par Response
 * - @ref CAPTURE_CHANNEL_ISP_RELEASE_RESP
 */
#define CAPTURE_CHANNEL_ISP_RELEASE_REQ		MK_U32(0x24)

/**
 * @brief ISP capture channel release response.
 *
 * This is a @ref CapCtrlMsgType "capture control message" received in
 * response to a @ref CAPTURE_CHANNEL_ISP_RELEASE_REQ message.
 *
 * @pre A @ref CAPTURE_CHANNEL_ISP_RELEASE_REQ message has been sent
 *      to the logical channel.
 *
 * @par Header
 * - @ref CAPTURE_CONTROL_MSG@b::@ref CAPTURE_MSG_HEADER "header"
 *   - @ref CAPTURE_MSG_HEADER::msg_id "msg_id" = @ref CAPTURE_CHANNEL_ISP_RELEASE_RESP
 *   - @ref CAPTURE_MSG_HEADER::channel_id "channel_id" =
 *     @ref CAPTURE_CHANNEL_ISP_RELEASE_REQ_MSG@b::@ref CAPTURE_MSG_HEADER "header"@b::@ref CAPTURE_MSG_HEADER::channel_id "channel_id"
 *
 * @par Payload
 * - @ref CAPTURE_CHANNEL_ISP_RELEASE_RESP_MSG
 */
#define CAPTURE_CHANNEL_ISP_RELEASE_RESP	MK_U32(0x25)
/** @} */
/** @} */

/**
 * @defgroup CapMsgType Message types for capture channel IVC messages.
 *
 * The capture IVC channel is established during boot using the
 * @ref CAMRTC_HSP_CH_SETUP command. The IVC channel is bi-directional
 * but the usage is asynchronous. The channel is used for sending
 * capture requests, ISP requests, and ISP program requests to RCE FW.
 * RCE FW in turn will send asynchronous status indications when the
 * requests complete. Multiple requests can be in progress in parallel.
 *
 * The overall message structure is defined by @ref CAPTURE_MSG and the
 * structure is the same for both requests and indications. Each message
 * consists of a common @ref CAPTURE_MSG_HEADER followed by message
 * specific data.
 *
 * The type of a message is identified by @ref CAPTURE_MSG_HEADER::msg_id.
 *
 * Requests on the same logical channel will be executed in the order they
 * were submitted and status indications will be sent back in the same order.
 * Request ordering is not guaranteed between different logical channels.
 * Requests and indications must specify the logical channel ID in the
 * @ref CAPTURE_MSG_HEADER::channel_id. The channel ID is assigned by
 * RCE FW when the logical channel is created.
 *
 * @{
 */

/**
 * @defgroup ViCapMsgType Message types for VI capture request messages and indications.
 *
 * Capture channel messages are used to submit capture requests and to
 * receive status indications pertaining to submitted requests.
 *
 * @{
 */

/**
 * @brief Submit a new capture request on a VI capture channel.
 *
 * This is a @ref CapMsgType "capture channel message" to
 * submit a VI capture request. The capture request provides
 * a reference to a @ref capture_descriptor in shared memory,
 * containing the detailed parameters for the capture.
 *
 * Capture completion is indicated to downstream engines by
 * incrementing the <em>progress syncpoint</em> (see @ref
 * capture_channel_config::progress_sp) a pre-calculated number of
 * times (2 + <em>number of sub-frames</em>). The first increment
 * occurs at @em start-of-frame and the last increment occurs at
 * @em end-of-frame. In between, the syncpoint is incremented once
 * for each completed @em subframe. The number of @em subframes
 * used in the capture is chosen by the client and is implemented
 * by programming flush points for the output buffer (see
 * @ref vi_channel_config::flush_enable,
 * @ref vi_channel_config::flush_periodic,
 * @ref vi_channel_config::flush, and
 * @ref vi_channel_config::flush_first. When the @em end-of-frame
 * increment occurs, the optional @ref capture_descriptor::engine_status
 * record is guaranteed to be populated if a buffer for it was
 * provided by the client.
 *
 * If @ref CAPTURE_FLAG_STATUS_REPORT_ENABLE
 * is set in @ref capture_descriptor::capture_flags, RCE will store
 * the capture status into @ref capture_descriptor::status. RCE will
 * also send a @ref CAPTURE_STATUS_IND message to indicate that the
 * capture was completed. The capture status record contains information
 * about the capture, such as CSI frame number, start-of-frame and
 * end-of-frame timestamps, a general success or fail status,
 * as well as a detailed record of any errors detected.
 *
 * If @ref CAPTURE_FLAG_ERROR_REPORT_ENABLE is set in
 * @ref capture_descriptor::capture_flags, RCE will send a
 * @ref CAPTURE_STATUS_IND message upon an error, even if
 * @ref CAPTURE_FLAG_STATUS_REPORT_ENABLE is not set.
 *
 * @pre A VI capture channel has been set up with
 *     @ref CAPTURE_CHANNEL_SETUP_REQ.
 *
 * @par Header
 * - @ref CAPTURE_MSG@b::@ref CAPTURE_MSG_HEADER "header"
 *   - @ref CAPTURE_MSG_HEADER::msg_id "msg_id" = @ref CAPTURE_REQUEST_REQ
 *   - @ref CAPTURE_MSG_HEADER::channel_id "channel_id" =
 *     @ref CAPTURE_CHANNEL_SETUP_RESP_MSG@b::@ref CAPTURE_MSG_HEADER "header"@b::@ref CAPTURE_MSG_HEADER::channel_id "channel_id"
 *
 * @par Payload
 * - @ref CAPTURE_REQUEST_REQ_MSG
 *
 * @par Response
 * - @ref CAPTURE_STATUS_IND (asynchronous)
 */
#define	CAPTURE_REQUEST_REQ			MK_U32(0x01)

/**
 * @brief Capture status indication.
 *
 * This is a @ref CapMsgType "capture channel message"
 * received in response to a @ref CAPTURE_REQUEST_REQ message
 * when the capture request has been completed and the
 * capture status record has been written into
 * @ref capture_descriptor::status.
 *
 * @pre A @ref CAPTURE_REQUEST_REQ has been sent with
 *      @ref CAPTURE_FLAG_STATUS_REPORT_ENABLE or
 *      @ref CAPTURE_FLAG_ERROR_REPORT_ENABLE set in
 *      @ref capture_descriptor::capture_flags.
 *
 * @par Header
 * - @ref CAPTURE_MSG@b::@ref CAPTURE_MSG_HEADER "header"
 *   - @ref CAPTURE_MSG_HEADER::msg_id "msg_id" = @ref CAPTURE_STATUS_IND
 *   - @ref CAPTURE_MSG_HEADER::channel_id "channel_id" =
 *     @ref CAPTURE_CHANNEL_SETUP_RESP_MSG@b::@ref CAPTURE_MSG_HEADER "header"@b::@ref CAPTURE_MSG_HEADER::channel_id "channel_id"
 *
 * @par Payload
 * - @ref CAPTURE_STATUS_IND_MSG
 */
#define	CAPTURE_STATUS_IND			MK_U32(0x02)

/**
 * @brief VI capture channel reset barrier indication.
 *
 * This is a @ref CapMsgType "capture channel message" sent on
 * the @em capture IVC channel in conjuncation with a
 * @ref CAPTURE_CHANNEL_RESET_REQ message sent on the
 * <em>capture-control</em> IVC channel to reset a VI capture channel.
 * This indication defines a boundary between capture requests
 * submitted before the reset request and capture requests submitted
 * after it. Capture requests submitted after the reset request are
 * not affected by the reset operation.
 *
 * @pre A VI capture channel has been set up with a
 *      @ref CAPTURE_CHANNEL_SETUP_REQ and a
 *      @ref CAPTURE_CHANNEL_RESET_REQ
 *      has been sent to the channel.
 *
 * @par Header
 * - @ref CAPTURE_MSG@b::@ref CAPTURE_MSG_HEADER "header"
 *   - @ref CAPTURE_MSG_HEADER::msg_id "msg_id" = @ref CAPTURE_RESET_BARRIER_IND
 *   - @ref CAPTURE_MSG_HEADER::channel_id "channel_id" =
 *     @ref CAPTURE_CHANNEL_SETUP_RESP_MSG@b::@ref CAPTURE_MSG_HEADER "header"@b::@ref CAPTURE_MSG_HEADER::channel_id "channel_id"
 */
#define	CAPTURE_RESET_BARRIER_IND		MK_U32(0x03)
/** @} */

/**
 * @defgroup IspCapMsgType Message types for ISP capture channel IVC messages.
 * @{
 */
#define CAPTURE_ISP_REQUEST_REQ			MK_U32(0x04)
#define CAPTURE_ISP_STATUS_IND			MK_U32(0x05)
#define CAPTURE_ISP_PROGRAM_REQUEST_REQ		MK_U32(0x06)
#define CAPTURE_ISP_PROGRAM_STATUS_IND		MK_U32(0x07)
#define CAPTURE_ISP_RESET_BARRIER_IND		MK_U32(0x08)
#define CAPTURE_ISP_EX_STATUS_IND		MK_U32(0x09)
/** @} */
/** @} */

/**
 * @brief Invalid message type. This can be used to respond to an invalid request.
 */
#define CAPTURE_MSG_ID_INVALID			MK_U32(0xFFFFFFFF)

/**
 * @brief Invalid channel id. Used when channel is not specified.
 */
#define CAPTURE_CHANNEL_ID_INVALID		MK_U32(0xFFFFFFFF)

typedef uint32_t capture_result;

/**
 * @defgroup CapErrorCodes Unsigned 32-bit return values for the capture-control IVC messages.
 * @{
 */
/** Request succeeded */
#define CAPTURE_OK				MK_U32(0)
/** Request failed: invalid parameter */
#define CAPTURE_ERROR_INVALID_PARAMETER		MK_U32(1)
/** Request failed: insufficient memory */
#define CAPTURE_ERROR_NO_MEMORY			MK_U32(2)
/** Request failed: service busy */
#define CAPTURE_ERROR_BUSY			MK_U32(3)
/** Request failed: not supported */
#define CAPTURE_ERROR_NOT_SUPPORTED		MK_U32(4)
/** Request failed: not initialized */
#define CAPTURE_ERROR_NOT_INITIALIZED		MK_U32(5)
/** Request failed: overflow */
#define CAPTURE_ERROR_OVERFLOW			MK_U32(6)
/** Request failed: no resource available */
#define CAPTURE_ERROR_NO_RESOURCES		MK_U32(7)
/** Request failed: timeout */
#define CAPTURE_ERROR_TIMEOUT			MK_U32(8)
/** Request failed: service in invalid state */
#define CAPTURE_ERROR_INVALID_STATE		MK_U32(9)
/** @} */

/** @brief Message data for @ref CAPTURE_CHANNEL_SETUP_REQ message */
struct CAPTURE_CHANNEL_SETUP_REQ_MSG {
	/** Capture channel configuration. */
	struct capture_channel_config	channel_config;
} CAPTURE_IVC_ALIGN;

/** @brief Message data for @ref CAPTURE_CHANNEL_SETUP_RESP message */
struct CAPTURE_CHANNEL_SETUP_RESP_MSG {
	/** Request result code. See @ref CapErrorCodes "result codes". */
	capture_result result;
	/** Capture channel identifier for the new channel [0, UINT32_MAX]. */
	uint32_t channel_id;
	/** 1-hot encoded bitmask indicating the allocated VI hardware
	 *  channel [0, 0xFFFFFFFFF ]. LSB is VI channel 0.
	 */
	uint64_t vi_channel_mask;
} CAPTURE_IVC_ALIGN;

/**
 * @defgroup CapResetFlags VI Capture channel reset flags
 * @{
 */
/** Reset the channel without waiting for frame-end first. */
#define CAPTURE_CHANNEL_RESET_FLAG_IMMEDIATE	MK_U32(0x01)
/** @} */

/** @brief Message data for @ref CAPTURE_CHANNEL_RESET_REQ message */
struct CAPTURE_CHANNEL_RESET_REQ_MSG {
	/** See @ref CapResetFlags "reset flags". */
	uint32_t reset_flags;
	/** Reserved */
	uint32_t pad__;
} CAPTURE_IVC_ALIGN;

/** @brief Message data for @ref CAPTURE_CHANNEL_RESET_RESP message */
struct CAPTURE_CHANNEL_RESET_RESP_MSG {
	/** Request result code. See @ref CapErrorCodes "result codes". */
	capture_result result;
	uint32_t pad__;
} CAPTURE_IVC_ALIGN;

/** @brief Message data for @ref CAPTURE_CHANNEL_RELEASE_REQ message */
struct CAPTURE_CHANNEL_RELEASE_REQ_MSG {
	/** Unused */
	uint32_t reset_flags;
	/** Reserved */
	uint32_t pad__;
} CAPTURE_IVC_ALIGN;

/** @brief Message data for @ref CAPTURE_CHANNEL_RELEASE_RESP message */
struct CAPTURE_CHANNEL_RELEASE_RESP_MSG {
	/** Request result code. See @ref CapErrorCodes "result codes". */
	capture_result result;
	/** Reserved */
	uint32_t pad__;
} CAPTURE_IVC_ALIGN;

/**
 * @brief Configure the piece-wise linear function used by the VI companding module.
 *
 * The companding table is shared by all capture channels and must be
 * configured before enabling companding for a specific capture. Each channel
 * can explicitly enable processing by the companding unit i.e the channels can
 * opt-out of the global companding config. See @ref CapErrorCodes "Capture request return codes"
 * for more details on the return values.
 */
struct CAPTURE_COMPAND_CONFIG_REQ_MSG {
	/** VI companding configuration */
	struct vi_compand_config compand_config;
} CAPTURE_IVC_ALIGN;

/**
 * @brief VI Companding unit configuration response message.
 *
 * Informs the client the status of VI companding unit configuration request.
 * A return value of CAPTURE_OK in the result field indicates the request
 * message succeeded. Any other value indicates an error.
 */
struct CAPTURE_COMPAND_CONFIG_RESP_MSG {
	/** Companding config setup result. See @ref CapErrorCodes "Return values". */
	capture_result result;
	uint32_t pad__;
} CAPTURE_IVC_ALIGN;

/**
 * @brief Configure the Phase Detection Auto Focus (PDAF) pattern.
 */
struct CAPTURE_PDAF_CONFIG_REQ_MSG {
	/** PDAF configuration data */
	struct vi_pdaf_config pdaf_config;
} CAPTURE_IVC_ALIGN;

/**
 * @brief Configure PDAF unit response message
 *
 * Returns the status PDAF unit configuration request.
 * A return value of CAPTURE_OK in the result field indicates the request
 * message succeeded. Any other value indicates an error. See
 * @ref CapErrorCodes "Capture request return codes" for more details on
 * the return values.
 */
struct CAPTURE_PDAF_CONFIG_RESP_MSG {
	/** PDAF config setup result. See @ref CapErrorCodes "Return values". */
	capture_result result;
	/** Reserved */
	uint32_t pad__;
} CAPTURE_IVC_ALIGN;

/*
 * @brief Enable SLVS-EC synchronization
 *
 * Enable the generation of XVS and XHS synchronization signals for a
 * SLVS-EC sensor.
 */
struct CAPTURE_SYNCGEN_ENABLE_REQ_MSG {
	/** Syncgen unit */
	uint32_t unit;
	/** Reserved */
	uint32_t pad__;
	/** VI SYNCGEN unit configuration */
	struct vi_syncgen_config syncgen_config;
} CAPTURE_IVC_ALIGN;

/**
 * @brief Enable SLVS-EC synchronization response message.
 *
 * Returns the status of enable SLVS-EC synchronization request.
 * A return value of CAPTURE_OK in the result field indicates the request
 * message succeeded. Any other value indicates an error. See
 * @ref CapErrorCodes "Capture request return codes" for more details on
 * the return values.
 */
struct CAPTURE_SYNCGEN_ENABLE_RESP_MSG {
	/** Syncgen unit */
	uint32_t unit;
	/** Syncgen enable request result. See @ref CapErrorCodes "Return values". */
	capture_result result;
} CAPTURE_IVC_ALIGN;

/**
 * @brief Disable SLVS-EC synchronization
 *
 * Disable the generation of XVS and XHS synchronization signals for a
 * SLVS-EC sensor.
 */
struct CAPTURE_SYNCGEN_DISABLE_REQ_MSG {
	/** Syncgen unit */
	uint32_t unit;
	/** See SyncgenDisableFlags "Syncgen disable flags" */
	uint32_t syncgen_disable_flags;

/**
 * @defgroup SyncgenDisableFlags Syncgen disable flags
 * @{
 */
/** Disable SYNCGEN without waiting for frame end */
#define CAPTURE_SYNCGEN_DISABLE_FLAG_IMMEDIATE	MK_U32(0x01)
/** @} */

} CAPTURE_IVC_ALIGN;

/**
 * @brief Disable SLVS-EC synchronization response message.
 *
 * Returns the status of the SLVS-EC synchronization request message.
 * A return value of CAPTURE_OK in the result field indicates the request
 * message succeeded. Any other value indicates an error. See
 * @ref CapErrorCodes "Capture request return codes" for more details on
 * the return values.
 */
struct CAPTURE_SYNCGEN_DISABLE_RESP_MSG {
	/** Syncgen unit */
	uint32_t unit;
	/** Syncgen disable request result .See @ref CapErrorCodes "Return values". */
	capture_result result;
} CAPTURE_IVC_ALIGN;


/**
 * @brief Open an NVCSI stream request message
 * @deprecated
 */
struct CAPTURE_PHY_STREAM_OPEN_REQ_MSG {
	/** See NvCsiStream "NVCSI stream id" */
	uint32_t stream_id;
	/** See NvCsiPort "NvCSI Port" */
	uint32_t csi_port;
	/** See @ref NvPhyType "NvCSI Physical stream type" */
	uint32_t phy_type;
	/** Reserved */
	uint32_t pad32__;
} CAPTURE_IVC_ALIGN;

/**
 * @brief NVCSI stream open response message
 * @deprecated
 */
struct CAPTURE_PHY_STREAM_OPEN_RESP_MSG {
	/** Stream open request status. See @ref CapErrorCodes "Return values". */
	uint32_t result;
	/** Reserved */
	uint32_t pad32__;
} CAPTURE_IVC_ALIGN;

/**
 * @brief NVCSI stream close request message
 * @deprecated
 */
struct CAPTURE_PHY_STREAM_CLOSE_REQ_MSG {
	/** NVCSI stream Id */
	uint32_t stream_id;
	/** NVCSI port */
	uint32_t csi_port;
	/** See @ref NvPhyType "NvCSI Physical stream type" */
	uint32_t phy_type;
	/** Reserved */
	uint32_t pad32__;
} CAPTURE_IVC_ALIGN;

/**
 * @brief NVCSI stream close response message
 * @deprecated
 */
struct CAPTURE_PHY_STREAM_CLOSE_RESP_MSG {
	/** Stream close request status. See @ref CapErrorCodes "Return values". */
	uint32_t result;
	/** Reserved */
	uint32_t pad32__;
} CAPTURE_IVC_ALIGN;

/**
 * @brief Physical stream dump registers request message. (Debug only)
 */
struct CAPTURE_PHY_STREAM_DUMPREGS_REQ_MSG {
	/** NVCSI stream Id */
	uint32_t stream_id;
	/** NVCSI port */
	uint32_t csi_port;
} CAPTURE_IVC_ALIGN;

/**
 * @brief Physical stream dump registers response message. (Debug only)
 */
struct CAPTURE_PHY_STREAM_DUMPREGS_RESP_MSG {
	/** Stream dump registers request status. See @ref CapErrorCodes "Return values". */
	uint32_t result;
	uint32_t pad32__;
} CAPTURE_IVC_ALIGN;

/**
 * @brief Set NVCSI stream configuration request message.
 */
struct CAPTURE_CSI_STREAM_SET_CONFIG_REQ_MSG {
	/** NVCSI stream Id */
	uint32_t stream_id;
	/** NVCSI port */
	uint32_t csi_port;
	/** @ref See NvCsiConfigFlags "NVCSI Configuration Flags" */
	uint32_t config_flags;
	/** Reserved */
	uint32_t pad32__;
	/** NVCSI super control and interface logic (SCIL aka brick) configuration */
	struct nvcsi_brick_config brick_config;
	/** NVCSI control and interface logic (CIL) partition configuration */
	struct nvcsi_cil_config cil_config;
	/** User-defined error configuration */
	struct nvcsi_error_config error_config;
} CAPTURE_IVC_ALIGN;

/**
 * @brief Set NVCSI stream configuration response message.
 */
struct CAPTURE_CSI_STREAM_SET_CONFIG_RESP_MSG {
	/** NVCSI stream config request status. See @ref CapErrorCodes "Return values". */
	uint32_t result;
	/** Reserved */
	uint32_t pad32__;
} CAPTURE_IVC_ALIGN;

/**
 * @brief Set NVCSI stream parameter request message.
 */
struct CAPTURE_CSI_STREAM_SET_PARAM_REQ_MSG {
	/** NVCSI stream Id */
	uint32_t stream_id;
	/** NVCSI stream virtual channel id */
	uint32_t virtual_channel_id;
	/** The parameter to set. See @ref NvCsiParamType "NVCSI Parameter Type" */
	uint32_t param_type;
	/** Reserved */
	uint32_t pad32__;
	/** @anon_union */
	union {
		/** Set DPCM config for an NVCSI stream @anon_union_member */
		struct nvcsi_dpcm_config dpcm_config;
		/** NVCSI watchdog timer config @anon_union_member */
		struct nvcsi_watchdog_config watchdog_config;
	};
} CAPTURE_IVC_ALIGN;

/**
 * @brief Set NVCSI stream parameter response message.
 */
struct CAPTURE_CSI_STREAM_SET_PARAM_RESP_MSG {
	/** NVCSI set stream parameter request status. See @ref CapErrorCodes "Return values". */
	uint32_t result;
	/** Reserved */
	uint32_t pad32__;
} CAPTURE_IVC_ALIGN;

/**
 * @brief NVCSI test pattern generator (TPG) stream config request message.
 */
struct CAPTURE_CSI_STREAM_TPG_SET_CONFIG_REQ_MSG {
	/** TPG configuration */
	union nvcsi_tpg_config tpg_config;
} CAPTURE_IVC_ALIGN;

/**
 * @brief NVCSI TPG stream config response message.
 */
struct CAPTURE_CSI_STREAM_TPG_SET_CONFIG_RESP_MSG {
	/** Set TPG config request status. See @ref CapErrorCodes "Return values". */
	uint32_t result;
	/** Reserved */
	uint32_t pad32__;
} CAPTURE_IVC_ALIGN;

/**
 * @brief Start NVCSI TPG streaming request message.
 */
struct CAPTURE_CSI_STREAM_TPG_START_REQ_MSG {
	/** NVCSI stream Id */
	uint32_t stream_id;
	/** NVCSI stream virtual channel id */
	uint32_t virtual_channel_id;
	/** TPG rate configuration */
	struct nvcsi_tpg_rate_config tpg_rate_config;
} CAPTURE_IVC_ALIGN;

/**
 * @brief Start NVCSI TPG streaming response message.
 */
struct CAPTURE_CSI_STREAM_TPG_START_RESP_MSG {
	/** TPG start request status. See @ref CapErrorCodes "Return values". */
	uint32_t result;
	/** Reserved */
	uint32_t pad32__;
} CAPTURE_IVC_ALIGN;


/**
 * @brief Start NVCSI TPG streaming at specified frame rate request message.
 *
 * This message is similar to CAPTURE_CSI_STREAM_TPG_START_REQ_MSG. Here the frame rate
 * and clock is specified using which the TPG rate config will be calculated.
 */
struct CAPTURE_CSI_STREAM_TPG_START_RATE_REQ_MSG {
	/** NVCSI stream Id */
	uint32_t stream_id;
	/** NVCSI stream virtual channel id */
	uint32_t virtual_channel_id;
	/** TPG frame rate in Hz */
	uint32_t frame_rate;
	/** Reserved */
	uint32_t pad32__;
} CAPTURE_IVC_ALIGN;

/**
 * @brief NVCSI TPG stream start at a specified frame rate response message.
 */
struct CAPTURE_CSI_STREAM_TPG_START_RATE_RESP_MSG {
	/** TPG start rate request status. See @ref CapErrorCodes "Return values". */
	uint32_t result;
	/** Reserved */
	uint32_t pad32__;
} CAPTURE_IVC_ALIGN;

/**
 * @defgroup NvCsiTpgGain gain ratio settings that can be set to frame generated by NVCSI TPG.
 * @{
 */
#define CAPTURE_CSI_STREAM_TPG_GAIN_RATIO_EIGHT_TO_ONE		MK_U8(0) /* 8:1 gain */
#define CAPTURE_CSI_STREAM_TPG_GAIN_RATIO_FOUR_TO_ONE		MK_U8(1) /* 4:1 gain */
#define CAPTURE_CSI_STREAM_TPG_GAIN_RATIO_TWO_TO_ONE		MK_U8(2) /* 2:1 gain */
#define CAPTURE_CSI_STREAM_TPG_GAIN_RATIO_NONE			MK_U8(3) /* 1:1 gain */
#define CAPTURE_CSI_STREAM_TPG_GAIN_RATIO_HALF			MK_U8(4) /* 0.5:1 gain */
#define CAPTURE_CSI_STREAM_TPG_GAIN_RATIO_ONE_FOURTH		MK_U8(5) /* 0.25:1 gain */
#define CAPTURE_CSI_STREAM_TPG_GAIN_RATIO_ONE_EIGHTH		MK_U8(6) /* 0.125:1 gain */
/** @} */

/**
 * @brief Apply gain ratio on specified VC of the desired CSI stream.
 *
 * This message is request to apply gain on specified vc, and it be
 * applied on next frame.
 */
struct CAPTURE_CSI_STREAM_TPG_APPLY_GAIN_REQ_MSG {
	/** NVCSI stream Id */
	uint32_t stream_id;
	/** NVCSI stream virtual channel id */
	uint32_t virtual_channel_id;
	/** Gain ratio */
	uint32_t gain_ratio;
	/** Reserved */
	uint32_t pad32__;
} CAPTURE_IVC_ALIGN;

/**
 * @brief NVCSI TPG stream start at a specified frame rate response message.
 */
struct CAPTURE_CSI_STREAM_TPG_APPLY_GAIN_RESP_MSG {
	/** TPG apply gain request status. See @ref CapErrorCodes "Return values". */
	uint32_t result;
	/** Reserved */
	uint32_t pad32__;
} CAPTURE_IVC_ALIGN;

/**
 * @brief Stop NVCSI TPG streaming request message.
 */
struct CAPTURE_CSI_STREAM_TPG_STOP_REQ_MSG {
	/** NVCSI stream Id */
	uint32_t stream_id;
	/** NVCSI stream virtual channel id */
	uint32_t virtual_channel_id;
} CAPTURE_IVC_ALIGN;

/**
 * @brief Stop NVCSI TPG streaming response message.
 */
struct CAPTURE_CSI_STREAM_TPG_STOP_RESP_MSG {
	/** Stop TPG steaming request status. See @ref CapErrorCodes "Return values". */
	uint32_t result;
	uint32_t pad32__;
} CAPTURE_IVC_ALIGN;

/**
 * @brief Max number of events
 */
#define VI_NUM_INJECT_EVENTS 10U

/**
 * @brief Event injection configuration.
 *
 * A capture request must be sent before this message
 */
struct CAPTURE_CHANNEL_EI_REQ_MSG {
	/** Event data used for event injection */
	struct event_inject_msg events[VI_NUM_INJECT_EVENTS];
	/** Number of error events */
	uint8_t num_events;
	/** Reserved */
	uint8_t pad__[7];
} CAPTURE_IVC_ALIGN;

/**
 * @brief Acknowledge Event Injection request
 */
struct CAPTURE_CHANNEL_EI_RESP_MSG {
	/** Stop TPG steaming request status. See @ref CapErrorCodes "Return values". */
	capture_result result;
	/** Reserved */
	uint32_t pad__;
} CAPTURE_IVC_ALIGN;

/**
 * @brief Event injection channel reset request.
 */
struct CAPTURE_CHANNEL_EI_RESET_REQ_MSG {
	/** Reserved */
	uint8_t pad__[8];
} CAPTURE_IVC_ALIGN;

/**
 * @brief Acknowledge Event injection channel reset request.
 */
struct CAPTURE_CHANNEL_EI_RESET_RESP_MSG {
	/** Event injection channel reset request result. See @ref CapErrorCodes "Return values". */
	capture_result result;
	/** Reserved */
	uint32_t pad__;
} CAPTURE_IVC_ALIGN;

/**
 * @addtogroup CapCtrlMsgType
 * @{
 */

/**
 * @defgroup PhyStreamMsgType NVCSI PHY Control Message Types
 * @{
 */
#define CAPTURE_PHY_STREAM_OPEN_REQ		MK_U32(0x36)
#define CAPTURE_PHY_STREAM_OPEN_RESP		MK_U32(0x37)
#define CAPTURE_PHY_STREAM_CLOSE_REQ		MK_U32(0x38)
#define CAPTURE_PHY_STREAM_CLOSE_RESP		MK_U32(0x39)
/** @} */

/**
 * @defgroup PhyStreamDebugMsgType NVCSI Phy Debug Message types (non-safety)
 * @{
 */
#define CAPTURE_PHY_STREAM_DUMPREGS_REQ		MK_U32(0x3C)
#define CAPTURE_PHY_STREAM_DUMPREGS_RESP	MK_U32(0x3D)
/** @} */

/**
 * @defgroup NvCsiMsgType NVCSI Configuration Message Types
 * @{
 */
#define CAPTURE_CSI_STREAM_SET_CONFIG_REQ	MK_U32(0x40)
#define CAPTURE_CSI_STREAM_SET_CONFIG_RESP	MK_U32(0x41)
#define CAPTURE_CSI_STREAM_SET_PARAM_REQ	MK_U32(0x42)
#define CAPTURE_CSI_STREAM_SET_PARAM_RESP	MK_U32(0x43)
/** @} */

/**
 * @defgroup NvCsiTpgMsgType NVCSI TPG Configuration Message Types
 * @{
 */
#define CAPTURE_CSI_STREAM_TPG_SET_CONFIG_REQ	MK_U32(0x44)
#define CAPTURE_CSI_STREAM_TPG_SET_CONFIG_RESP	MK_U32(0x45)
#define CAPTURE_CSI_STREAM_TPG_START_REQ	MK_U32(0x46)
#define CAPTURE_CSI_STREAM_TPG_START_RESP	MK_U32(0x47)
#define CAPTURE_CSI_STREAM_TPG_STOP_REQ		MK_U32(0x48)
#define CAPTURE_CSI_STREAM_TPG_STOP_RESP	MK_U32(0x49)
#define CAPTURE_CSI_STREAM_TPG_START_RATE_REQ	MK_U32(0x4A)
#define CAPTURE_CSI_STREAM_TPG_START_RATE_RESP	MK_U32(0x4B)
#define CAPTURE_CSI_STREAM_TPG_APPLY_GAIN_REQ	MK_U32(0x4C)
#define CAPTURE_CSI_STREAM_TPG_APPLY_GAIN_RESP	MK_U32(0x4D)
/** @} */

/**
 * @defgroup ViEiCapCtrlMsgType VI Event Injection Message Types (non-safety)
 * @{
 */
#define CAPTURE_CHANNEL_EI_REQ			MK_U32(0x50)
#define CAPTURE_CHANNEL_EI_RESP			MK_U32(0x51)
#define CAPTURE_CHANNEL_EI_RESET_REQ		MK_U32(0x52)
#define CAPTURE_CHANNEL_EI_RESET_RESP		MK_U32(0x53)
/** @} */

/**
 * @addtogroup ViCapCtrlMsgType
 * @{
 */
/**
 * @brief Override VI CHANSEL safety error masks.
 *
 * This is a @ref CapCtrlMsgType "capture control message" to
 * override the default VI CHANSEL safety error masks on all
 * active VI units.
 *
 * @pre The capture-control IVC channel has been set up during
 *      boot using the @ref CAMRTC_HSP_CH_SETUP command.
 *
 * @par Header
 * - @ref CAPTURE_CONTROL_MSG@b::@ref CAPTURE_MSG_HEADER "header"
 *   - @ref CAPTURE_MSG_HEADER::msg_id "msg_id" = @ref CAPTURE_HSM_CHANSEL_ERROR_MASK_REQ
 *   - @ref CAPTURE_MSG_HEADER::transaction "transaction" = <em>unique ID</em>
 *
 * @par Payload
 * - @ref CAPTURE_HSM_CHANSEL_ERROR_MASK_REQ_MSG
 *
 * @par Response
 * - @ref CAPTURE_HSM_CHANSEL_ERROR_MASK_RESP
 */
#define CAPTURE_HSM_CHANSEL_ERROR_MASK_REQ	MK_U32(0x54)

/**
 * @brief Override VI CHANSEL safety error masks response.
 *
 * This is a @ref CapCtrlMsgType "capture control message" received in
 * response to a @ref CAPTURE_HSM_CHANSEL_ERROR_MASK_REQ message.
 *
 * @pre A @ref CAPTURE_HSM_CHANSEL_ERROR_MASK_REQ message has been sent.
 *
 * @par Header
 * - @ref CAPTURE_CONTROL_MSG@b::@ref CAPTURE_MSG_HEADER "header"
 *   - @ref CAPTURE_MSG_HEADER::msg_id "msg_id" = @ref CAPTURE_HSM_CHANSEL_ERROR_MASK_RESP
 *   - @ref CAPTURE_MSG_HEADER::transaction "transaction" =
 *     @ref CAPTURE_HSM_CHANSEL_ERROR_MASK_REQ_MSG@b::@ref CAPTURE_MSG_HEADER "header"@b::@ref CAPTURE_MSG_HEADER::transaction "transaction"
 *
 * @par Payload
 * - @ref CAPTURE_HSM_CHANSEL_ERROR_MASK_RESP_MSG
 */
#define CAPTURE_HSM_CHANSEL_ERROR_MASK_RESP	MK_U32(0x55)
/** @} */
/** @} */

/**
 * @addtogroup ViCapCtrlMsgs
 * @{
 */
/** @brief Message data for @ref CAPTURE_HSM_CHANSEL_ERROR_MASK_REQ message */
struct CAPTURE_HSM_CHANSEL_ERROR_MASK_REQ_MSG {
	/** VI CHANSEL safety error mask configuration */
	struct vi_hsm_chansel_error_mask_config hsm_chansel_error_config;
} CAPTURE_IVC_ALIGN;

/** @brief Message data for @ref CAPTURE_HSM_CHANSEL_ERROR_MASK_RESP message */
struct CAPTURE_HSM_CHANSEL_ERROR_MASK_RESP_MSG {
	/** Request result. See @ref CapErrorCodes "result codes". */
	capture_result result;
	/** Reserved */
	uint32_t pad__;
} CAPTURE_IVC_ALIGN;
/** @} */

/** @brief Message data for @ref CAPTURE_CHANNEL_ISP_SETUP_REQ message */
struct CAPTURE_CHANNEL_ISP_SETUP_REQ_MSG {
	/** ISP channel configuration. */
	struct capture_channel_isp_config	channel_config;
} CAPTURE_IVC_ALIGN;

/** @brief Message data for @ref CAPTURE_CHANNEL_ISP_SETUP_RESP message */
struct CAPTURE_CHANNEL_ISP_SETUP_RESP_MSG {
	/** Request result code. See @ref CapErrorCodes "result codes". */
	capture_result result;
	/** ISP channel identifier for the new channel [0, UINT32_MAX]. */
	uint32_t channel_id;
} CAPTURE_IVC_ALIGN;

typedef struct CAPTURE_CHANNEL_ISP_RESET_REQ_MSG
			CAPTURE_CHANNEL_ISP_RESET_REQ_MSG;
typedef struct CAPTURE_CHANNEL_ISP_RESET_RESP_MSG
			CAPTURE_CHANNEL_ISP_RESET_RESP_MSG;
typedef struct CAPTURE_CHANNEL_ISP_RELEASE_REQ_MSG
			CAPTURE_CHANNEL_ISP_RELEASE_REQ_MSG;
typedef struct CAPTURE_CHANNEL_ISP_RELEASE_RESP_MSG
			CAPTURE_CHANNEL_ISP_RELEASE_RESP_MSG;

/** @brief Message data for @ref CAPTURE_CHANNEL_ISP_RESET_REQ message */
struct CAPTURE_CHANNEL_ISP_RESET_REQ_MSG {
	/** Unused */
	uint32_t reset_flags;
	/** Reserved */
	uint32_t pad__;
} CAPTURE_IVC_ALIGN;

/** @brief Message data for @ref CAPTURE_CHANNEL_ISP_RESET_RESP message */
struct CAPTURE_CHANNEL_ISP_RESET_RESP_MSG {
	/** Request result code. See @ref CapErrorCodes "result codes". */
	capture_result result;
	/** Reserved */
	uint32_t pad__;
} CAPTURE_IVC_ALIGN;

/** @brief Message data for @ref CAPTURE_CHANNEL_ISP_RELEASE_REQ message */
struct CAPTURE_CHANNEL_ISP_RELEASE_REQ_MSG {
	/** Unused */
	uint32_t reset_flags;
	/** Reserved */
	uint32_t pad__;
} CAPTURE_IVC_ALIGN;

/** @brief Message data for @ref CAPTURE_CHANNEL_ISP_RELEASE_RESP message */
struct CAPTURE_CHANNEL_ISP_RELEASE_RESP_MSG {
	/** Request result code. See @ref CapErrorCodes "result codes". */
	capture_result result;
	/** Reserved */
	uint32_t pad__;
} CAPTURE_IVC_ALIGN;

/**
 * @brief Message frame for capture-control IVC channel.
 *
 * This structure describes a common message format for all capture channel
 * IVC messages. The message format includes a message header that is common
 * to all messages, followed by a message-specific message data.  The message
 * data is represented as an anonymous union of message structures.
 */
struct CAPTURE_CONTROL_MSG {
	/** Common capture control channel message header */
	struct CAPTURE_MSG_HEADER header;
	/** @anon_union */
	union {
		/** @anon_union_member */
		/** Message data for @ref CAPTURE_CHANNEL_SETUP_REQ message */
		struct CAPTURE_CHANNEL_SETUP_REQ_MSG channel_setup_req;
		/** @anon_union_member */
		/** Message data for @ref CAPTURE_CHANNEL_SETUP_RESP message */
		struct CAPTURE_CHANNEL_SETUP_RESP_MSG channel_setup_resp;
		/** @anon_union_member */
		/** Message data for @ref CAPTURE_CHANNEL_RESET_REQ message */
		struct CAPTURE_CHANNEL_RESET_REQ_MSG channel_reset_req;
		/** @anon_union_member */
		/** Message data for @ref CAPTURE_CHANNEL_RESET_RESP message */
		struct CAPTURE_CHANNEL_RESET_RESP_MSG channel_reset_resp;
		/** @anon_union_member */
		/** Message data for @ref CAPTURE_CHANNEL_RELEASE_REQ message */
		struct CAPTURE_CHANNEL_RELEASE_REQ_MSG channel_release_req;
		/** @anon_union_member */
		/** Message data for @ref CAPTURE_CHANNEL_RELEASE_RESP message */
		struct CAPTURE_CHANNEL_RELEASE_RESP_MSG channel_release_resp;
		/** @anon_union_member */
		/** Message data for @ref CAPTURE_COMPAND_CONFIG_REQ message (non-safety) */
		struct CAPTURE_COMPAND_CONFIG_REQ_MSG compand_config_req;
		/** @anon_union_member */
		/** Message data for @ref CAPTURE_COMPAND_CONFIG_RESP message (non-safety) */
		struct CAPTURE_COMPAND_CONFIG_RESP_MSG compand_config_resp;
		/** @anon_union_member */
		/** Message data for @ref CAPTURE_PDAF_CONFIG_REQ message (non-safety) */
		struct CAPTURE_PDAF_CONFIG_REQ_MSG pdaf_config_req;
		/** @anon_union_member */
		/** Message data for @ref CAPTURE_PDAF_CONFIG_RESP message (non-safety) */
		struct CAPTURE_PDAF_CONFIG_RESP_MSG pdaf_config_resp;
		/** @anon_union_member */
		/** Message data for @ref CAPTURE_SYNCGEN_ENABLE_REQ message (non-safety) */
		struct CAPTURE_SYNCGEN_ENABLE_REQ_MSG syncgen_enable_req;
		/** @anon_union_member */
		/** Message data for @ref CAPTURE_SYNCGEN_ENABLE_RESP message (non-safety) */
		struct CAPTURE_SYNCGEN_ENABLE_RESP_MSG syncgen_enable_resp;
		/** @anon_union_member */
		/** Message data for @ref CAPTURE_SYNCGEN_DISABLE_REQ message (non-safety) */
		struct CAPTURE_SYNCGEN_DISABLE_REQ_MSG syncgen_disable_req;
		/** @anon_union_member */
		/** Message data for @ref CAPTURE_SYNCGEN_DISABLE_RESP message (non-safety) */
		struct CAPTURE_SYNCGEN_DISABLE_RESP_MSG syncgen_disable_resp;

		/** @anon_union_member */
		/** Message data for @ref CAPTURE_PHY_STREAM_OPEN_REQ message */
		struct CAPTURE_PHY_STREAM_OPEN_REQ_MSG phy_stream_open_req;
		/** @anon_union_member */
		/** Message data for @ref CAPTURE_PHY_STREAM_OPEN_RESP message */
		struct CAPTURE_PHY_STREAM_OPEN_RESP_MSG phy_stream_open_resp;
		/** @anon_union_member */
		/** Message data for @ref CAPTURE_PHY_STREAM_CLOSE_REQ message */
		struct CAPTURE_PHY_STREAM_CLOSE_REQ_MSG phy_stream_close_req;
		/** @anon_union_member */
		/** Message data for @ref CAPTURE_PHY_STREAM_CLOSE_RESP message */
		struct CAPTURE_PHY_STREAM_CLOSE_RESP_MSG phy_stream_close_resp;
		/** @anon_union_member */
		/** Message data for @ref CAPTURE_PHY_STREAM_DUMPREGS_REQ message (non-safety) */
		struct CAPTURE_PHY_STREAM_DUMPREGS_REQ_MSG
			phy_stream_dumpregs_req;
		/** @anon_union_member */
		/** Message data for @ref CAPTURE_PHY_STREAM_DUMPREGS_RESP message (non-safety) */
		struct CAPTURE_PHY_STREAM_DUMPREGS_RESP_MSG
			phy_stream_dumpregs_resp;

		/** @anon_union_member */
		/** Message data for @ref CAPTURE_CSI_STREAM_SET_CONFIG_REQ message */
		struct CAPTURE_CSI_STREAM_SET_CONFIG_REQ_MSG
			csi_stream_set_config_req;
		/** @anon_union_member */
		/** Message data for @ref CAPTURE_CSI_STREAM_SET_CONFIG_RESP message */
		struct CAPTURE_CSI_STREAM_SET_CONFIG_RESP_MSG
			csi_stream_set_config_resp;
		/** @anon_union_member */
		/** Message data for @ref CAPTURE_CSI_STREAM_SET_PARAM_REQ message */
		struct CAPTURE_CSI_STREAM_SET_PARAM_REQ_MSG
			csi_stream_set_param_req;
		/** @anon_union_member */
		/** Message data for @ref CAPTURE_CSI_STREAM_SET_PARAM_RESP message */
		struct CAPTURE_CSI_STREAM_SET_PARAM_RESP_MSG
			csi_stream_set_param_resp;
		/** @anon_union_member */
		/** Message data for @ref CAPTURE_CSI_STREAM_TPG_SET_CONFIG_REQ message (non-safety) */
		struct CAPTURE_CSI_STREAM_TPG_SET_CONFIG_REQ_MSG
			csi_stream_tpg_set_config_req;
		/** @anon_union_member */
		/** Message data for @ref CAPTURE_CSI_STREAM_TPG_SET_CONFIG_RESP message (non-safety) */
		struct CAPTURE_CSI_STREAM_TPG_SET_CONFIG_RESP_MSG
			csi_stream_tpg_set_config_resp;
		/** @anon_union_member */
		/** Message data for @ref CAPTURE_CSI_STREAM_TPG_START_REQ message (non-safety) */
		struct CAPTURE_CSI_STREAM_TPG_START_REQ_MSG
			csi_stream_tpg_start_req;
		/** @anon_union_member */
		/** Message data for @ref CAPTURE_CSI_STREAM_TPG_START_RESP message (non-safety) */
		struct CAPTURE_CSI_STREAM_TPG_START_RESP_MSG
			csi_stream_tpg_start_resp;
		/** @anon_union_member */
		/** Message data for @ref CAPTURE_CSI_STREAM_TPG_STOP_REQ message (non-safety) */
		struct CAPTURE_CSI_STREAM_TPG_STOP_REQ_MSG
			csi_stream_tpg_stop_req;
		/** @anon_union_member */
		/** Message data for @ref CAPTURE_CSI_STREAM_TPG_STOP_RESP message (non-safety) */
		struct CAPTURE_CSI_STREAM_TPG_STOP_RESP_MSG
			csi_stream_tpg_stop_resp;
		/** @anon_union_member */
		/** Message data for @ref CAPTURE_CSI_STREAM_TPG_START_RATE_REQ message (non-safety) */
		struct CAPTURE_CSI_STREAM_TPG_START_RATE_REQ_MSG
			csi_stream_tpg_start_rate_req;
		/** @anon_union_member */
		/** Message data for @ref CAPTURE_CSI_STREAM_TPG_START_RATE_RESP message (non-safety) */
		struct CAPTURE_CSI_STREAM_TPG_START_RATE_RESP_MSG
			csi_stream_tpg_start_rate_resp;
		/** @anon_union_member */
		/** Message data for @ref CAPTURE_CSI_STREAM_TPG_APPLY_GAIN_REQ message (non-safety) */
		struct CAPTURE_CSI_STREAM_TPG_APPLY_GAIN_REQ_MSG
			csi_stream_tpg_apply_gain_req;
		/** @anon_union_member */
		/** Message data for @ref CAPTURE_CSI_STREAM_TPG_APPLY_GAIN_RESP message (non-safety) */
		struct CAPTURE_CSI_STREAM_TPG_APPLY_GAIN_RESP_MSG
			csi_stream_tpg_apply_gain_resp;

		/** @anon_union_member */
		/** Message data for @ref CAPTURE_CHANNEL_EI_REQ message (non-safety) */
		struct CAPTURE_CHANNEL_EI_REQ_MSG ei_req;
		/** @anon_union_member */
		/** Message data for @ref CAPTURE_CHANNEL_EI_RESP message (non-safety) */
		struct CAPTURE_CHANNEL_EI_RESP_MSG ei_resp;
		/** @anon_union_member */
		/** Message data for @ref CAPTURE_CHANNEL_EI_RESET_REQ message (non-safety) */
		struct CAPTURE_CHANNEL_EI_RESET_REQ_MSG ei_reset_req;
		/** @anon_union_member */
		/** Message data for @ref CAPTURE_CHANNEL_EI_RESET_RESP message (non-safety) */
		struct CAPTURE_CHANNEL_EI_RESET_RESP_MSG ei_reset_resp;

		/** @anon_union_member */
		/** Message data for @ref CAPTURE_CHANNEL_ISP_SETUP_REQ message */
		struct CAPTURE_CHANNEL_ISP_SETUP_REQ_MSG channel_isp_setup_req;
		/** @anon_union_member */
		/** Message data for @ref CAPTURE_CHANNEL_ISP_SETUP_RESP message */
		struct CAPTURE_CHANNEL_ISP_SETUP_RESP_MSG channel_isp_setup_resp;
		/** @anon_union_member */
		/** Message data for @ref CAPTURE_CHANNEL_ISP_RESET_REQ message */
		struct CAPTURE_CHANNEL_ISP_RESET_REQ_MSG channel_isp_reset_req;
		/** @anon_union_member */
		/** Message data for @ref CAPTURE_CHANNEL_ISP_RESET_RESP message */
		struct CAPTURE_CHANNEL_ISP_RESET_RESP_MSG channel_isp_reset_resp;
		/** @anon_union_member */
		/** Message data for @ref CAPTURE_CHANNEL_ISP_RELEASE_REQ message */
		struct CAPTURE_CHANNEL_ISP_RELEASE_REQ_MSG channel_isp_release_req;
		/** @anon_union_member */
		/** Message data for @ref CAPTURE_CHANNEL_ISP_RELEASE_RESP message */
		struct CAPTURE_CHANNEL_ISP_RELEASE_RESP_MSG channel_isp_release_resp;

		/** @anon_union_member */
		/** Message data for @ref CAPTURE_HSM_CHANSEL_ERROR_MASK_REQ message */
		struct CAPTURE_HSM_CHANSEL_ERROR_MASK_REQ_MSG hsm_chansel_mask_req;
		/** @anon_union_member */
		/** Message data for @ref CAPTURE_HSM_CHANSEL_ERROR_MASK_RESP message */
		struct CAPTURE_HSM_CHANSEL_ERROR_MASK_RESP_MSG hsm_chansel_mask_resp;
	};
} CAPTURE_IVC_ALIGN;

/** @brief Message data for @ref CAPTURE_REQUEST_REQ message. */
struct CAPTURE_REQUEST_REQ_MSG {
	/**
	 * Buffer index identifying the location of a capture descriptor:
	 * struct @ref capture_descriptor *desc =
	 * @ref capture_channel_config::requests "requests" +
	 * buffer_index * @ref capture_channel_config::request_size "request_size";
	 */
	uint32_t buffer_index;
	/** Reserved */
	uint32_t pad__;
} CAPTURE_IVC_ALIGN;

/** @brief Message data for @ref CAPTURE_STATUS_IND message. */
struct CAPTURE_STATUS_IND_MSG {
	/**
	 * Buffer index identifying the location of a capture descriptor:
	 * struct @ref capture_descriptor *desc =
	 * @ref capture_channel_config::requests "requests" +
	 * buffer_index * @ref capture_channel_config::request_size "request_size";
	 */
	uint32_t buffer_index;
	/** Reserved */
	uint32_t pad__;
} CAPTURE_IVC_ALIGN;


/**
 * @brief Send new isp_capture request on a capture channel.
 *
 * The request contains channel identifier and the capture sequence
 * number (ring-buffer index), which are required to schedule the
 * isp capture request.
 * The actual capture programming is stored in isp_capture_descriptor,
 * stored in DRAM ring buffer, which includes the sequence, ISP
 * surfaces' details, surface related configs, ISP PB2 iova, input prefences,
 * and isp_capture status written by RCE.
 *
 * NvCapture UMD allocates the pool of isp_capture descriptors in setup call,
 * where each isp_capture_desc is followed by corresponding PB2 memory
 * (ATOM aligned).
 * RCE would generate the PB2 using surface details found in isp_capture
 * descriptor.
 * The ring-buffer (pool) would look like below:
 *
 * [isp_capture_desc][PB2][isp_capture_desc][PB2][isp_capture_desc]...
 *
 * The isp_capture_descriptor with buffer_index=N can be located within
 * the ring buffer as follows:
 *
 * isp_capture_descriptor *desc = requests + buffer_index * request_size;
 *
 * Note, here request_size = sizeof (isp_capture_descriptor) + sizeof (PB2).
 *
 * UMD fills isp_capture_desc and submits the request to KMD which pins the
 * surfaces and PB and then does the in-place replacement with iovas' within
 * isp_capture_descriptor.
 * KMD then sends the isp_capture request to RCE over capture ivc channel.
 *
 * The isp capture request message is asynchronous. Capture completion is
 * indicated by incrementing the progress syncpoint a pre-calculated
 * number of times = <number of sub-frames>. The progress-syncpoint is
 * used to synchronize with down-stream engines. This model assumes that
 * the capture client knows the number of subframes used in the capture and has
 * programmed the ISP accordingly.
 * All stats completion are indicated by incrementing stats progress syncpoint
 * a number of times = <num-stats-enabled>.
 *
 * If the flag CAPTURE_FLAG_ISP_STATUS_REPORT_ENABLE is set in the isp
 * capture descriptor, RCE will store the capture status into status field
 * of the descriptor. RCE will also send a CAPTURE_ISP_STATUS_IND
 * message to indicate that capture has completed.
 *
 * If the flag CAPTURE_FLAG_ISP_ERROR_REPORT_ENABLE is set, RCE will send a
 * CAPTURE_ISP_STATUS_IND upon an error, even if
 * CAPTURE_FLAG_ISP_STATUS_REPORT_ENABLE is not set.
 *
 * Typedef-ed CAPTURE_REQUEST_REQ_MSG.
 *
 * The buffer_index field is isp_capture_descriptor index in ring buffer.
 */
typedef struct CAPTURE_REQUEST_REQ_MSG CAPTURE_ISP_REQUEST_REQ_MSG;

/**
 * @brief ISP Capture status indication.
 *
 * The message is sent after the capture status record has been
 * written into the capture request descriptor.
 *
 * The buffer_index	in this case is identifying the ISP capture descriptor.
 */
typedef struct CAPTURE_STATUS_IND_MSG CAPTURE_ISP_STATUS_IND_MSG;

/**
 * @brief Extended ISP capture status indication.
 *
 * The message is sent after the capture status record has been
 * written into the capture request descriptor.
 */
struct CAPTURE_ISP_EX_STATUS_IND_MSG {
	/** Buffer index identifying ISP process descriptor. */
	uint32_t process_buffer_index;
	/** Buffer index identifying ISP program descriptor. */
	uint32_t program_buffer_index;
} CAPTURE_IVC_ALIGN;

/**
 * @brief Send new isp_program request on a capture ivc channel.
 *
 * The request contains channel identifier and the program sequence
 * number (ring-buffer index).
 * The actual programming details is stored in isp_program
 * descriptor, which includes the offset to isp_program
 * buffer (which has PB1 containing ISP HW settings), sequence,
 * settings-id, activation-flags, isp_program buffer size, iova's
 * of ISP PB1 and isp_program status written by RCE.
 *
 * NvCapture UMD allocates the pool of isp_program descriptors in setup call,
 * where each isp_pgram_descriptor is followed by corresponding isp_program
 * buffer (ATOM aligned).
 * The ring-buffer (pool) would look like below:
 *
 * [isp_prog_desc][isp_program][isp_prog_desc][isp_program][isp_prog_desc]...
 *
 * The isp_program_descriptor with buffer_index=N can be located within
 * the ring buffer as follows:
 *
 * isp_program_descriptor *desc = programs + buffer_index * program_size;
 *
 * Note, program_size = sizeof (isp_program_descriptor) + sizeof (isp_program).
 *
 * NvISP fills these and submits the isp_program request to KMD which pins the
 * PB and then does the in-place replacement with iova within
 * isp_program_descriptor.
 * KMD then sends the isp_program request to RCE over capture ivc channel.
 *
 * The sequence is the frame_id which tells RCE, that the given isp_program
 * must be used from that frame_id onwards until UMD provides new one.
 * So RCE will use the sequence field to select the correct isp_program from
 * the isp_program descriptors' ring buffer for given frame request and will
 * keep on using it for further frames until the new isp_program (desc) is
 * provided to be used.
 * RCE populates both matched isp_program (reads from isp program desc) and
 * isp capture descriptor and forms single task descriptor for given frame
 * request and feeds it to falcon, which further programs it to ISP.
 *
 * settings_id is unique id for isp_program, NvCapture and RCE will use
 * the ring buffer array index as settings_id.
 * It can also be used to select the correct isp_program for the given
 * frame, in that case, UMD writes this unique settings_id to sensor's
 * scratch register, and sensor will send back it as part of embedded data,
 * when the given settings/gains are applied on that particular frame
 * coming from sensor.
 *
 * RCE reads this settings_id back from embedded data and uses it to select
 * the corresponding isp_program from the isp_program desc ring buffer.
 * The activation_flags tells the RCE which id (sequence or settings_id) to
 * use to select correct isp_program for the given frame.
 *
 * As same isp_program can be used for multiple frames, it can not be freed
 * when the frame capture is done. RCE will send a separate status
 * indication CAPTURE_ISP_PROGRAM_STATUS_IND message to CCPEX to notify
 * that the given isp_program is no longer in use and can be freed or reused.
 * settings_id (ring-buffer index) field is used to uniquely identify the
 * correct isp_program.
 * RCE also writes the isp_program status in isp program descriptor.
 *
 * Typedef-ed CAPTURE_REQUEST_REQ_MSG.
 *
 * The buffer_index field is the isp_program descriptor index in ring buffer.
 */
typedef struct CAPTURE_REQUEST_REQ_MSG CAPTURE_ISP_PROGRAM_REQUEST_REQ_MSG;

/**
 * @brief ISP program status indication.
 *
 * The message is sent to notify CCPLEX about the isp_program which is expired
 * so UMD client can free or reuse it.
 *
 * Typedef-ed CAPTURE_STATUS_IND_MSG.
 *
 * The buffer_index field in this case is identifying ISP program descriptor.
 */
typedef struct CAPTURE_STATUS_IND_MSG CAPTURE_ISP_PROGRAM_STATUS_IND_MSG;

/**
 * @brief Message frame for capture IVC channel.
 */
struct CAPTURE_MSG {
	struct CAPTURE_MSG_HEADER header;
	/** @anon_union */
	union {
		/** @anon_union_member */
		struct CAPTURE_REQUEST_REQ_MSG capture_request_req;
		/** @anon_union_member */
		struct CAPTURE_STATUS_IND_MSG capture_status_ind;

		/** @anon_union_member */
		CAPTURE_ISP_REQUEST_REQ_MSG capture_isp_request_req;
		/** @anon_union_member */
		CAPTURE_ISP_STATUS_IND_MSG capture_isp_status_ind;
		/** @anon_union_member */
		struct CAPTURE_ISP_EX_STATUS_IND_MSG capture_isp_ex_status_ind;

		/** @anon_union_member */
		CAPTURE_ISP_PROGRAM_REQUEST_REQ_MSG
				capture_isp_program_request_req;
		/** @anon_union_member */
		CAPTURE_ISP_PROGRAM_STATUS_IND_MSG
				capture_isp_program_status_ind;
	};
} CAPTURE_IVC_ALIGN;

#pragma GCC diagnostic ignored "-Wpadded"

#endif /* INCLUDE_CAMRTC_CAPTURE_MESSAGES_H */
