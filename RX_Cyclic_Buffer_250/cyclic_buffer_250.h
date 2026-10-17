/* For Arduino Nano/Uno or similar
*  Cyclic Buffer is a circular FIFO with a write index and a read index.
*  It's designed to store signal timings from a 433/868Mhz receiver pin via interrupt.
*  Timings are stored after each Space/Mark transition, or OFF/ON transition, if you prefer.
*  Two durations in microseconds representing the Mark/ON time and the Space/OFF time are stored.
*  
*  There are various devices that send data in different formats and therefore a calling program
*  can read the durations and determine what the coding pattern is for the device being received and
*  decode the data as required. The advantage of a large buffer is it allows printing of detailed
*  signal data without losing information. However, if the buffer does fill, the writing is paused until the
*  buffer has been completely emptied which should minimise data corruption by ensuring that complete 
*  signal bursts can be captured and analysed.
*/


#ifndef cyclic_buffer_250_h
#define cyclic_buffer_250_h

#include "Arduino.h"

#define CYCLIC_BUFFER_SIZE 250 // Max number of entries is CYCLIC_BUFFER_SIZE-1

// MARK & SPACE MICROSECOND COUNTS
struct mark_space_t {
    uint16_t mark;
    uint16_t space;
};

// Cyclic Mark Space Buffer
// Each location stores an 8-bit Mark duration and an 8-bit Space duration for later analysis

class Cyclic_MS_Buffer {
        // The cyclic buffer is empty when the read_index == write_index
        // Each index points to the next location that will be read from/written to.
        // The buffer is full when the write_index points to the location before the
        // read_index, i.e. when there are (buffer_size - 1) entries.


        // Attributes
    private:
        mark_space_t bit_buffer[CYCLIC_BUFFER_SIZE];
        volatile uint8_t write_index;      // points to next location to write to
        volatile uint8_t read_index;       // points to next location to read from
        volatile bool pause_write;         // Stops more data going into buffer until buffer completely empty

        uint8_t  lookahead_index;     // Used to look forward in buffer without affecting read_index
        uint32_t last_interrupt_time;
        uint8_t  interrupt_pin;
        uint8_t  buffer_full_count; // Diagnostic: Count of number of times buffer was full


        // =================== METHODS ========================
    public:
        // Constructor
        Cyclic_MS_Buffer(uint8_t interrupt_pin);

        bool data_available();
        uint8_t new_data_entry();
        bool buffer_full();
        uint8_t buffer_entries();
        uint8_t index_difference(uint8_t first_index, uint8_t second_index);
        mark_space_t read_buffer(); // Get the mark/space values at read_index and advance the index
        void advance_read_index();  // Advance the read_index without a read
        void skip_to_lookahead();   // Advance to the read_index to lookahead_index
        void reset_lookahead();     // Reset lookahead_index to current read_index
        mark_space_t get_buffer();  // Get the mark/space values at read_index without advancing the index
        mark_space_t get_buffer(uint8_t index);      // Get the mark/space values at the given index
        bool lookahead_data_available();             // New data has been written since last get_lookahead_data()
        mark_space_t get_lookahead_data();           // Returns data at the lookahead_index and advances index
        void write_mark(uint16_t count);  // Write a mark duration to the buffer without advancing the write index
        void write_space(uint16_t count); // Write a space duration to the buffer, advancing the write index
        void write_data(mark_space_t data); // Write a mark/space duration to the buffer, advancing the write index
        void write_data(uint16_t mark_count, uint16_t space_count); // Write a mark/space duration to the buffer, advancing the write index
        void reset_buffer();
        uint8_t next_write_location();
        void interrupt_handler();


        // ====== Printing methods for debugging

        void print_buffer_entries(); // Print entries in the buffer
        void print_buffer(uint8_t start, uint8_t finish); // Print the bit_buffer in two lines of mark & space
        void print_all_buffer();
        void print_data_debug(int16_t data);
        void print_data_debug(uint16_t data, bool sign = false);
        void print_spaces(uint8_t spaces);
        void print_buffer_full_count();

};

#endif