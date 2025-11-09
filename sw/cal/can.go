package main

import (
	"context"
	"time"

	"go.einride.tech/can"

	"git.samanthony.xyz/can_gauge_interface/sw/cal/canbus"
)

const (
	stdMask = 0x7FF
	extMask = 0x1FFFFFFF

	timeout    = 1 * time.Second
	maxRetries = 8
)

func sendCtrlFrame[C can.FrameMarshaler, R can.FrameUnmarshaler](cmd C, req can.FrameMarshaler, reply R, bus canbus.Bus, isReply func(R) bool, verify func(C, R) bool) error {
	cmdFrame, err := cmd.MarshalFrame()
	if err != nil {
		return err
	}

	for retry := 0; retry < maxRetries; retry++ {
		// Write
		ctx, _ := context.WithTimeout(context.Background(), timeout)
		if err := bus.Send(ctx, cmdFrame); err != nil {
			return err
		}

		// Read back
		reqFrame, err := req.MarshalFrame()
		if err != nil {
			return err
		}
		ctx, _ = context.WithTimeout(context.Background(), timeout)
		if err := bus.Send(ctx, reqFrame); err != nil {
			return err
		}
		ctx, _ = context.WithTimeout(context.Background(), timeout)
		replyFrame, err := bus.Receive(ctx)
		if err != nil {
			return err
		}
		err = reply.UnmarshalFrame(replyFrame)
		if err == errWrongId || !isReply(reply) {
			continue
		} else if err != nil {
			return err
		}

		// Verify
		if verify(cmd, reply) {
			return nil
		}
	}
	return errVerifyFail
}
