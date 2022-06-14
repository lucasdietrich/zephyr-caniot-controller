# CAN Rosbustness

- Check that there is frame lost for sure (using frame counter/sequence number in CANIOT device and compare, detect gaps)
  - Maybe using extended ID (CAN protocol) could be used to detect gaps for a retransmission mecanism, and a better way to manipulate the `handle`.
    In this context it would be possible to send several commands to the same device at the same time (but maybe in doesn't make sense, as the device only
    process 1 frame at a time).
- Provide a retransmission mechanism, QOS for sensitive commands/data (such as alarm controller, etc...)
- By activating `DEBUG_CANIOT_EMULATE_RESPONSE`, software seems to be robust
  - 0 frames lost of 1000
- Evaluate CPU usage, optimize threads priority for minimum delay

1 frame lost over 100
```
39/3570 Time 0.041  <Response [200]>
39/3571 Time 0.069  <Response [200]>
39/3572 Time 0.016  <Response [200]>
39/3573 Time 0.049  <Response [200]>
39/3574 Time 0.044  <Response [200]>
39/3575 Time 0.089  <Response [200]>
39/3576 Time 0.097  <Response [200]>
39/3577 Time 0.096  <Response [200]>
39/3578 Time 0.097  <Response [200]>
39/3579 Time 0.021  <Response [200]>
39/3580 Time 0.047  <Response [200]>
39/3581 Time 0.086  <Response [200]>
39/3582 Time 0.024  <Response [200]>
```