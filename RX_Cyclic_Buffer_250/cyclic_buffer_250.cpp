/* A Version of cyclic_buffer with a CYCLIC_BUFFER_SIZE of 250 */

#include "Arduino.h"
#include "cyclic_buffer_250.h"


// Constructor
Cyclic_MS_Buffer::Cyclic_MS_Buffer(uint8_t int_pin) {
    interrupt_pin = int_pin;
    pause_write = false;
    reset_buffer();
}

// Methods

// Is there data available to be read?
bool Cyclic_MS_Buffer::data_available() {
    return bool(read_index != write_index);
}


// Return the number of entries between two indexes
uint8_t Cyclic_MS_Buffer::index_difference(uint8_t first_index, uint8_t second_index) {

    if (second_index >= first_index) return (second_index - first_index);
        else return CYCLIC_BUFFER_SIZE - first_index + second_index;
}

// Return the number of used locations in the buffer
uint8_t Cyclic_MS_Buffer::buffer_entries() {
    return index_difference (read_index, write_index);
}

// Is the buffer full?
bool Cyclic_MS_Buffer::buffer_full() {  
    if (write_index > read_index) return ((write_index - read_index) == 1);
    else return (write_index == 0) and (read_index == (CYCLIC_BUFFER_SIZE - 1));
}


// Get the mark/space values at read_index and advance the index
// Caller must check data_available.
mark_space_t Cyclic_MS_Buffer::read_buffer() {
    mark_space_t data = bit_buffer[read_index];
    advance_read_index();
    return data;
}


// Advances the read_index without a read but will not pass write_index
void Cyclic_MS_Buffer::advance_read_index() {
    if (read_index != write_index) {
        read_index++;
        if (read_index >= CYCLIC_BUFFER_SIZE) read_index = 0;
    }
}

/* The lookahead pointer allows examination of data in the buffer without affecting the read_index.
*  Data can be examnined while the buffer continues to be be filled by the write_index but no more writes
*  will be allowed once the write_index reachess the read_index. Any further data received will be lost
*  but data in the buffer will be protected from being overwritten.
*  This allows data to be pre-processed before reading.
*/

// Dump the data between the read_index and the lookahead index
void Cyclic_MS_Buffer::skip_to_lookahead() {
    read_index = lookahead_index;
}

// Reset the lookahead index to the read_index to allow further processing without moving the read_index
void Cyclic_MS_Buffer::reset_lookahead() {
    lookahead_index = read_index;
}

// Returns true if data available at the lookahead_index
bool Cyclic_MS_Buffer::lookahead_data_available() {
    return (lookahead_index != write_index);
}

// Returns the data at the lookahead_index and advances the index. If the lookahead_index is aready at the 
// write_index it will return the same data and the index will not be advanced.
// Must be used with lookahead_data_available() to avoid duplicate data
mark_space_t Cyclic_MS_Buffer::get_lookahead_data() {
    mark_space_t data = bit_buffer[lookahead_index];
    if (lookahead_index != write_index) {
        lookahead_index++;
        if (lookahead_index >= CYCLIC_BUFFER_SIZE) lookahead_index = 0;
    }
    return data;
}


// Get the mark/space values at read_index without advancing the index
mark_space_t Cyclic_MS_Buffer::get_buffer() {
    return bit_buffer[read_index];
}


// Get the mark/space values at the given index (syntactic sugar!)
mark_space_t Cyclic_MS_Buffer::get_buffer(uint8_t index) {
    return bit_buffer[index];
}

// Return the previous read_index
uint8_t Cyclic_MS_Buffer::previous_read_index() {
    return previous_read_index(1);
}

// Return a negative offset from the current read_index
uint8_t Cyclic_MS_Buffer::previous_read_index(uint8_t offset) {
    return offset_index(-offset, read_index);
}

// // Return a offset from the provided index
uint8_t Cyclic_MS_Buffer::offset_index(int16_t offset, int16_t index) {    
    index += offset;
    while (index >= CYCLIC_BUFFER_SIZE) index -= CYCLIC_BUFFER_SIZE;
    while (index < 0) index += CYCLIC_BUFFER_SIZE;
    return (uint8_t)index;
}

// Backup read_index by one
uint8_t Cyclic_MS_Buffer::backup_read_index() {
    return read_index = previous_read_index();
}

// Backup the read_index and return the value at the index.
mark_space_t Cyclic_MS_Buffer::read_backwards() {
    backup_read_index();
    get_buffer();
}

// Write a mark duration to the buffer without advancing the write index
void Cyclic_MS_Buffer::write_mark(uint16_t duration) {
    if (!pause_write) bit_buffer[write_index].mark = duration;
}

// Write a space duration to the buffer, advancing the write index
void Cyclic_MS_Buffer::write_space(uint16_t duration) {
    if (!pause_write) bit_buffer[write_index++].space = duration;
    if (write_index >= CYCLIC_BUFFER_SIZE) write_index = 0;
}

// Write a mark/space duration to the buffer, advancing the write index
void Cyclic_MS_Buffer::write_data(mark_space_t data) {
    if (!pause_write) bit_buffer[write_index++] = data;
    if (write_index >= CYCLIC_BUFFER_SIZE) write_index = 0;
}

// Write a mark/space duration to the buffer, advancing the write index
void Cyclic_MS_Buffer::write_data(uint16_t mark_duration, uint16_t space_duration) {
    if (!pause_write) {
        write_mark(mark_duration);
        write_space(space_duration); 
    }
}

void Cyclic_MS_Buffer::reset_buffer() {
    write_index = 0; // points to next location to write to
    read_index = 0; // points to next location to read from
    pause_write = false;
}

uint8_t Cyclic_MS_Buffer::next_write_location() {
    if (write_index == CYCLIC_BUFFER_SIZE-1) return 0;
    else return write_index+1;
}

void Cyclic_MS_Buffer::interrupt_handler() {

    const uint32_t timer = micros();   // Freeze the clock
    const boolean input_level = digitalRead(interrupt_pin);
    
    uint32_t duration = (timer - last_interrupt_time);
    last_interrupt_time = timer;

    if (pause_write) {
        if (read_index == write_index) pause_write = false;  // Buffer has emptied. Clear pause_write
        else return; 
    }

    else if (next_write_location() == read_index) {  // Buffer full
        pause_write = true; // Halt writing to the buffer until the buffer has emptied
        buffer_full_count++; // Monitor number of times buffer has been full
        return;
    }

    if (duration > 0xFFFF) duration = 0xFFFF;  // So not to exceed 16 bits = 65.5mSecs

    if (input_level) write_space(duration); // Just transitioned high; duration is last space
        else write_mark(duration);
}

// ====== Printing methods for debugging


void Cyclic_MS_Buffer::print_buffer_entries() {
    print_buffer(read_index, write_index);
}


void Cyclic_MS_Buffer::print_buffer(uint8_t start, uint8_t finish) {

    Serial.print(F("Buffer entries:"));Serial.print(buffer_entries());
    Serial.print(F(" R:")); Serial.print(read_index);
    Serial.print(F(" W:")); Serial.println(write_index);
    // Print the bit_buffer in two lines of mark & space
    // from bit_buffer[start] up to and including bit_buffer[finish]
    for (uint8_t i = start; i < finish; i++) { // Print mark bit_buffer
        print_data_debug(get_buffer(i).mark);
    }
    Serial.println();
    for (uint8_t i = start; i < finish; i++) { // Print mark bit_buffer
        print_data_debug(get_buffer(i).space);
    }
    Serial.println();
}

void Cyclic_MS_Buffer::print_all_buffer() {
    print_buffer(0, CYCLIC_BUFFER_SIZE);
}

void Cyclic_MS_Buffer::print_data_debug(int16_t data) {

    if (data < 0) print_data_debug((uint16_t) abs(data), true);
    else {print_data_debug((uint16_t) data);}
}

void Cyclic_MS_Buffer::print_data_debug(uint16_t data, bool sign) {

    uint8_t spaces;
    if (data < 10) spaces  = 5;
    else if (data < 100) spaces = 4;
    else if (data < 1000) spaces = 3;
    else if (data < 10000) spaces = 2;
    else spaces = 1;

    if (sign) spaces -= 1;
    print_spaces(spaces);
    if (sign) Serial.print("-");
    Serial.print(data);
}

void Cyclic_MS_Buffer::print_spaces(uint8_t spaces) {
    while ( spaces-- > 0 ) {
        Serial.print(' ');
    }
}

void Cyclic_MS_Buffer::print_buffer_full_count() {
    Serial.print(buffer_full_count);
}



