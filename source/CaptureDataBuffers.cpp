#include "capture_structs.hpp"
#include <string.h>

CaptureDataBuffers::CaptureDataBuffers() {
	last_curr_in = 0;
	curr_writer_pos = -1;
	for(int i = 0; i < NUM_CONCURRENT_DATA_BUFFER_READERS; i++)
		curr_reader_pos[i] = -1;
	for(int i = 0; i < NUM_CONCURRENT_DATA_BUFFERS; i++) {
		num_readers[i] = 0;
		for(int j = 0; j < NUM_CONCURRENT_DATA_BUFFER_READERS; j++)
			has_read_data[i][j] = true;
	}
}

static int reader_to_index(CaptureReaderType reader_type) {
	switch(reader_type) {
		case CAPTURE_READER_VIDEO:
			return 0;
		case CAPTURE_READER_AUDIO:
			return 1;
		default:
			return 0;
	}
}

CaptureDataSingleBuffer* CaptureDataBuffers::GetReaderBuffer(CaptureReaderType reader_type) {
	int index = reader_to_index(reader_type);
	CaptureDataSingleBuffer* retval = NULL;
	access_mutex.lock();
	if(curr_reader_pos[index] != -1)
		retval = &buffers[curr_reader_pos[index]];
	else if(!has_read_data[last_curr_in][index]) {
		has_read_data[last_curr_in][index] = true;
		num_readers[last_curr_in] += 1;
		retval = &buffers[last_curr_in];
		curr_reader_pos[index] = last_curr_in;
	}
	access_mutex.unlock();
	return retval;
}

CaptureDataSingleBuffer* CaptureDataBuffers::GetWriterBuffer() {
	CaptureDataSingleBuffer* retval = NULL;
	access_mutex.lock();
	if(curr_writer_pos != -1)
		retval = &buffers[curr_writer_pos];
	else
		for(int i = 0; i < NUM_CONCURRENT_DATA_BUFFERS; i++)
			if((num_readers[i] == 0) && (i != last_curr_in)) {
				retval = &buffers[i];
				curr_writer_pos = i;
				for(int j = 0; j < NUM_CONCURRENT_DATA_BUFFER_READERS; j++)
					has_read_data[i][j] = false;
				break;
			}
	access_mutex.unlock();
	return retval;
}

void CaptureDataBuffers::ReleaseReaderBuffer(CaptureReaderType reader_type) {
	int index = reader_to_index(reader_type);
	access_mutex.lock();
	if(curr_reader_pos[index] != -1) {
		num_readers[curr_reader_pos[index]] -= 1;
		curr_reader_pos[index] = -1;
	}
	access_mutex.unlock();
}

void CaptureDataBuffers::ReleaseWriterBuffer() {
	access_mutex.lock();
	if(curr_writer_pos != -1) {
		buffers[curr_writer_pos].inner_index = inner_index++;
		last_curr_in = curr_writer_pos;
		curr_writer_pos = -1;
	}
	access_mutex.unlock();
}

void CaptureDataBuffers::WriteToBuffer(CaptureReceived* buffer, uint64_t read, double time_in_buf, CaptureDevice* device, CaptureScreensType capture_type, size_t offset) {
	if(offset >= read)
		return;
	CaptureDataSingleBuffer* target = this->GetWriterBuffer();
	// How did we end here?!
	if(target == NULL)
		return;
	memcpy(&target->capture_buf, ((uint8_t*)buffer) + offset, read - offset);
	// Make sure to also copy the extra needed data, if any
	if((device->cc_type == CAPTURE_CONN_USB) && (!device->is_3ds))
		memcpy(&target->capture_buf.usb_received_old_ds.frameinfo, &buffer->usb_received_old_ds.frameinfo, sizeof(buffer->usb_received_old_ds.frameinfo));
	target->read = read;
	target->time_in_buf = time_in_buf;
	target->capture_type = capture_type;
	this->ReleaseWriterBuffer();
}

void CaptureDataBuffers::WriteToBuffer(CaptureReceived* buffer, uint64_t read, double time_in_buf, CaptureDevice* device, size_t offset) {
	return this->WriteToBuffer(buffer, read, time_in_buf, device, CAPTURE_SCREENS_BOTH, offset);
}
