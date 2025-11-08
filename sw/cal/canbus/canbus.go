package canbus

import (
	"context"
	"net"
	"strings"
	"time"

	"go.einride.tech/can"
	"go.einride.tech/can/pkg/socketcan"
)

const timeout = 1 * time.Second

type Bus struct {
	conn        net.Conn
	receiver    *socketcan.Receiver
	transmitter *socketcan.Transmitter

	tx <-chan txChan

	rx    <-chan can.Frame
	rxErr <-chan error

	cancel context.CancelFunc
}

type txChan struct {
	request chan<- transmission
	err     <-chan error
}

type transmission struct {
	context.Context
	can.Frame
}

func Connect(dev string) (Bus, error) {
	conn, err := socketcan.Dial("can", dev)
	if err != nil {
		return Bus{}, err
	}

	receiver := socketcan.NewReceiver(conn)
	transmitter := socketcan.NewTransmitter(conn)
	tx := make(chan txChan)
	rx := make(chan can.Frame)
	rxErr := make(chan error)
	ctx, cancel := context.WithCancel(context.Background())

	go transmit(ctx, transmitter, tx)
	go receive(ctx, receiver, rx, rxErr)

	return Bus{conn, receiver, transmitter, tx, rx, rxErr, cancel}, nil
}

func transmit(ctx context.Context, transmitter *socketcan.Transmitter, tx chan<- txChan) {
	defer close(tx)

	for {
		reqc := make(chan transmission)
		errc := make(chan error)
		select {
		case tx <- txChan{reqc, errc}:
			req := <-reqc
			errc <- req.transmit(transmitter)
			close(reqc)
			close(errc)
		case <-ctx.Done():
			close(reqc)
			close(errc)
			return
		}
	}
}

func (t transmission) transmit(transmitter *socketcan.Transmitter) error {
	ctx, _ := context.WithTimeout(context.Background(), timeout)
	for {
		select {
		case <-ctx.Done():
			return ctx.Err()
		default:
		}
		err := transmitter.TransmitFrame(t.Context, t.Frame)
		if err == nil {
			return nil // success
		} else if strings.HasSuffix(err.Error(), "no buffer space available") {
			continue // retry
		} else {
			return err
		}
	}
}

func receive(ctx context.Context, receiver *socketcan.Receiver, rx chan<- can.Frame, rxErr chan<- error) {
	defer close(rx)
	defer close(rxErr)

	for receiver.Receive() {
		frame := receiver.Frame()
		select {
		case rx <- frame:
		case <-ctx.Done():
			return
		}
	}
	rxErr <- receiver.Err()
}

func (b Bus) Close() {
	b.cancel()
	b.transmitter.Close()
	b.receiver.Close()
	b.conn.Close()
}

func (b Bus) Send(ctx context.Context, frame can.Frame) error {
	c := <-b.tx
	c.request <- transmission{ctx, frame}
	return <-c.err
}

func (b Bus) Receive(ctx context.Context) (can.Frame, error) {
	select {
	case frame := <-b.rx:
		return frame, nil
	case <-ctx.Done():
		return can.Frame{}, ctx.Err()
	}
}

func (b Bus) TryReceive() (can.Frame, bool) {
	select {
	case frame := <-b.rx:
		return frame, true
	default:
		return can.Frame{}, false
	}
}
