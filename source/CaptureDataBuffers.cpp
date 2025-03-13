#include "capture_structs.hpp"
#include <string.h>

CaptureDataBuffers::CaptureDataBuffers() {
	last_curr_in = 0;
	for(int i = 0; i < NUM_CONCURRENT_DATA_BUFFER_WRITERS; i++) {
		curr_writer_pos[i] = -1;
		is_being_written_to[i] = false;
	}
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

static bool is_writer_index_valid(int index) {
	if((index < 0) || (index >= NUM_CONCURRENT_DATA_BUFFER_WRITERS))
		return false;
	return true;
}

CaptureDataSingleBuffer* CaptureDataBuffers::GetReaderBuffer(CaptureReaderType reader_type) {
	int index = reader_to_index(reader_type);
	CaptureDataSingleBuffer* retval = NULL;
	if(curr_reader_pos[index] != -1)
		return &buffers[curr_reader_pos[index]];
	access_mutex.lock();
	if(!has_read_data[last_curr_in][index]) {
		has_read_data[last_curr_in][index] = true;
		num_readers[last_curr_in] += 1;
		retval = &buffers[last_curr_in];
		curr_reader_pos[index] = last_curr_in;
	}
	access_mutex.unlock();
	return retval;
}

CaptureDataSingleBuffer* CaptureDataBuffers::GetWriterBuffer(int index) {
	if(!is_writer_index_valid(index))
		return NULL;
	if(curr_writer_pos[index] != -1)
		return &buffers[curr_writer_pos[index]];
	CaptureDataSingleBuffer* retval = NULL;
	access_mutex.lock();
	for(int i = 0; i < NUM_CONCURRENT_DATA_BUFFERS; i++)
		if((num_readers[i] == 0) && (i != last_curr_in) && (!is_being_written_to[i])) {
			retval = &buffers[i];
			is_being_written_to[i] = true;
			curr_writer_pos[index] = i;
			for(int j = 0; j < NUM_CONCURRENT_DATA_BUFFER_READERS; j++)
				has_read_data[i][j] = true;
			break;
		}
	access_mutex.unlock();
	return retval;
}

void CaptureDataBuffers::ReleaseReaderBuffer(CaptureReaderType reader_type) {
	int index = reader_to_index(reader_type);
	if(curr_reader_pos[index] == -1)
		return;
	access_mutex.lock();
	num_readers[curr_reader_pos[index]] -= 1;
	curr_reader_pos[index] = -1;
	access_mutex.unlock();
}

void CaptureDataBuffers::ReleaseWriterBuffer(int index, bool update_last_curr_in) {
	if(!is_writer_index_valid(index))
		return;
	if(curr_writer_pos[index] == -1)
		return;
	access_mutex.lock();
	if(update_last_curr_in) {
		for(int j = 0; j < NUM_CONCURRENT_DATA_BUFFER_READERS; j++)
			has_read_data[curr_writer_pos[index]][j] = false;
		last_curr_in = curr_writer_pos[index];
	}
	is_being_written_to[curr_writer_pos[index]] = false;
	curr_writer_pos[index] = -1;
	access_mutex.unlock();
}

void CaptureDataBuffers::WriteToBuffer(CaptureReceived* buffer, uint64_t read, double time_in_buf, CaptureDevice* device, CaptureScreensType capture_type, size_t offset, int index) {
	if(offset >= read)
		return;
	if(!is_writer_index_valid(index))
		return;
	CaptureDataSingleBuffer* target = this->GetWriterBuffer(index);
	// How did we end here?!
	if(target == NULL)
		return;
	if(buffer != NULL) {
		memcpy(&target->capture_buf, ((uint8_t*)buffer) + offset, read - offset);
		// Make sure to also copy the extra needed data, if any
		if((device->cc_type == CAPTURE_CONN_USB) && (!device->is_3ds))
			memcpy(&target->capture_buf.usb_received_old_ds.frameinfo, &buffer->usb_received_old_ds.frameinfo, sizeof(buffer->usb_received_old_ds.frameinfo));
		target->unused_offset = 0;
	}
	else
		target->unused_offset = offset;
	target->read = read - offset;
	target->time_in_buf = time_in_buf;
	target->capture_type = capture_type;
	this->ReleaseWriterBuffer(index);
}

void CaptureDataBuffers::WriteToBuffer(CaptureReceived* buffer, uint64_t read, double time_in_buf, CaptureDevice* device, CaptureScreensType capture_type, int index) {
	return this->WriteToBuffer(buffer, read, time_in_buf, device, capture_type, 0, index);
}

void CaptureDataBuffers::WriteToBuffer(CaptureReceived* buffer, uint64_t read, double time_in_buf, CaptureDevice* device, size_t offset, int index) {
	return this->WriteToBuffer(buffer, read, time_in_buf, device, CAPTURE_SCREENS_BOTH, offset, index);
}

void CaptureDataBuffers::WriteToBuffer(CaptureReceived* buffer, uint64_t read, double time_in_buf, CaptureDevice* device, int index) {
	return this->WriteToBuffer(buffer, read, time_in_buf, device, CAPTURE_SCREENS_BOTH, 0, index);
}
