// Flags for the status register
#define STATUS_M1_I1 0
#define STATUS_M1_I2 1
#define STATUS_M2_I1 2
#define STATUS_M2_I2 3
#define STATUS_INT1 4
#define STATUS_INT2 5

// Flags for the direction register
#define DIR_M1_CCW 0
#define DIR_M1_CW 1
#define DIR_M2_CCW 2
#define DIR_M2_CW 3

// Flags for the inopts register
#define INOPT_PULLUP_I1 0
#define INOPT_PULLUP_I2 1
#define INOPT_INVERT_I1 2
#define INOPT_INVERT_I2 3
#define INOPT_LIMIT_I1 4
#define INOPT_LIMIT_I2 5

// Flags for the int_masks register
#define IMASK_LIMIT_M1_I1 0
#define IMASK_LIMIT_M1_I2 1
#define IMASK_LIMIT_M2_I1 2
#define IMASK_LIMIT_M2_I2 3

#define NUM_REGISTERS 10

typedef union {
    uint8_t bytes[NUM_REGISTERS];
    struct {
        uint8_t slave_addr;
        uint8_t status;         // Input and interrupt status register
        uint8_t direction;      // Motor direction register
        uint8_t reserved;       // Reserved for future use
        uint8_t speed[2];      // Motor speeds, 0=off/idle, 255=full speed
        uint8_t inopts[2];      // Input options/flags registers
        uint8_t int_mask[2];   // Interrupt masks registers
    } __attribute__((__packed__)) reg;
} register_t;
