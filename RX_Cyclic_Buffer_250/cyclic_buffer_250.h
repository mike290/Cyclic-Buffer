


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
        mark_space_t read_backwards();               // Backup the read_index and return the value at the index.
        uint8_t previous_read_index();               // Return the previous read_index
        uint8_t previous_read_index(uint8_t offset); // Return a negative offset from the current read_index
        uint8_t offset_index(int16_t offset, int16_t index); // Return a offset from the provided index
        uint8_t backup_read_index();                 // Backup read_index by one
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