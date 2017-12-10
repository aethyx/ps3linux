
CC=gcc
CFLAGS=-Wall -O2 -g -fno-strict-aliasing
LDFLAGS=
LIB=-lcrypto -lssl

all: bd_auth bd_enable_buffer_write bd_mode_sense bd_read_buffer \
	bd_write_buffer bd_read_capacity bd_read_policy bd_read_profile \
	bd_send_security_cmd_for_check bd_set_auto_req_sense_mode \
	bd_enable_sacd bd_update_fw

bd_auth: bd_auth.o bd_util.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIB)

bd_enable_buffer_write: bd_enable_buffer_write.o bd_util.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIB)

bd_mode_sense: bd_mode_sense.o bd_util.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIB)

bd_read_buffer: bd_read_buffer.o bd_util.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIB)

bd_write_buffer: bd_write_buffer.o bd_util.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIB)

bd_read_capacity: bd_read_capacity.o bd_util.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIB)

bd_read_policy: bd_read_policy.o bd_util.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIB)

bd_read_profile: bd_read_profile.o bd_util.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIB)

bd_send_security_cmd_for_check: bd_send_security_cmd_for_check.o bd_util.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIB)

bd_set_auto_req_sense_mode: bd_set_auto_req_sense_mode.o bd_util.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIB)

bd_enable_sacd: bd_enable_sacd.o bd_util.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIB)

bd_update_fw: bd_update_fw.o bd_util.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIB)

%.o: %.c
	$(CC) $(CFLAGS) -c $<

.PHONY: clean
clean:
	rm -f bd_auth.o bd_auth
	rm -f bd_enable_buffer_write.o bd_enable_buffer_write
	rm -f bd_mode_sense.o bd_mode_sense
	rm -f bd_read_buffer.o bd_read_buffer
	rm -f bd_write_buffer.o bd_write_buffer
	rm -f bd_read_capacity.o bd_read_capacity
	rm -f bd_read_policy.o bd_read_policy
	rm -f bd_read_profile.o bd_read_profile
	rm -f bd_send_security_cmd_for_check.o bd_send_security_cmd_for_check
	rm -f bd_set_auto_req_sense_mode.o bd_set_auto_req_sense_mode
	rm -f bd_enable_sacd.o bd_enable_sacd
	rm -f bd_update_fw.o bd_update_fw
	rm -f bd_util.o
