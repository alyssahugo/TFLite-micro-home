/* systolic_full_test.c
 * Phase 1: CSR register write + readback verification
 * Phase 2: DMA load test data into weight + ifmap SPADs
 * Phase 3: Run systolic array, poll o_done
 * Phase 4: DMA read output SPAD via adapter then DDR, print results
 */

#include <stdint.h>

// Base addresses (from Vivado Address Editor)
#define CSR_BASE             0x00001000
#define DMA_BASE             0x00010000
#define OUT_SPAD_BASE        0x00020000   // axi_output_read_adapter  — assign in Address Editor!
#define SPAD_WRITE_ADAPTER   0xC0000000   // axi_spad_write_adapt_0   — from Address Editor

// DDR regions (test locations lang)
#define DDR_WEIGHT_SRC  0x80080000   // test weight data placed here
#define DDR_IFMAP_SRC   0x80090000   // test ifmap data placed here
#define DDR_RESULT      0x80200000   // output DMA writes here

// CSR register offsets
#define CSR_CONV_MODE   0x00
#define CSR_P_MODE      0x04
#define CSR_I_SIZE      0x08
#define CSR_I_C_SIZE    0x0C
#define CSR_O_C_SIZE    0x10
#define CSR_O_SIZE      0x14
#define CSR_STRIDE      0x18
#define CSR_DEPTH_MULT  0x1C
#define CSR_I_START     0x20
#define CSR_I_END       0x24
#define CSR_W_START     0x28
#define CSR_W_END       0x2C
#define CSR_CTRL        0x30   // write-only: bit0=START, bit1=reg_clear
#define CSR_STATUS      0x34   // read-only:  bit0=o_done
#define CSR_QUANT_SH    0x38
// 0x3C quant_mult  — NOT YET IN RTL, will return 0xB0BACAFE
// 0x44 zero_point  — NOT YET IN RTL, will return 0xB0BACAFE

// DMA register offsets (channel N base = DMA_BASE + N*0x20) 
#define DMA_SRC(ch)    (DMA_BASE + (ch)*0x20 + 0x00)
#define DMA_DST(ch)    (DMA_BASE + (ch)*0x20 + 0x04)
#define DMA_LEN(ch)    (DMA_BASE + (ch)*0x20 + 0x08)
#define DMA_CTRL(ch)   (DMA_BASE + (ch)*0x20 + 0x0C)
#define DMA_STAT(ch)   (DMA_BASE + (ch)*0x20 + 0x10)
#define DMA_SPAD(ch)   (DMA_BASE + (ch)*0x20 + 0x14)

// DMA SPAD_SEL encoding
#define SPAD_WEIGHTS  0   // 000
#define SPAD_IFMAPS   1   // 001
#define SPAD_BIAS     2   // 010
#define SPAD_SCALE    3   // 011
#define SPAD_SHIFT    4   // 100

// DMA channels
#define CH_WEIGHTS    0
#define CH_IFMAPS     1
#define CH_OUTPUT     3

// Helpers 
static inline void     wr32(uint32_t a, uint32_t v) { *(volatile uint32_t*)a = v; }
static inline uint32_t rd32(uint32_t a)              { return *(volatile uint32_t*)a; }

static int  pass_count = 0;
static int  fail_count = 0;

static void check(const char *name, uint32_t got, uint32_t expected) {
    if (got == expected) {
        printf("  [PASS] %-20s = 0x%08x\n", name, got);
        pass_count++;
    } else {
        printf("  [FAIL] %-20s got=0x%08x expected=0x%08x\n", name, got, expected);
        fail_count++;
    }
}

// Wait for DMA DONE (bit1), then W1C clear it
static void dma_wait(int ch) {
    while (!(rd32(DMA_STAT(ch)) & 0x2))
        ;
    wr32(DMA_STAT(ch), 0x2);
}

// PHASE 1: CSR write + readback
static void phase1_csr_verify(void) {
    printf("\n=== PHASE 1: CSR Register Write/Readback ===\n");

    // Write known test values to every writable register
    wr32(CSR_BASE + CSR_CONV_MODE,  0x1);
    wr32(CSR_BASE + CSR_P_MODE,     0x2);
    wr32(CSR_BASE + CSR_I_SIZE,     0xAA);
    wr32(CSR_BASE + CSR_I_C_SIZE,   0xBB);
    wr32(CSR_BASE + CSR_O_C_SIZE,   0xCC);
    wr32(CSR_BASE + CSR_O_SIZE,     0xDD);
    wr32(CSR_BASE + CSR_STRIDE,     0x3);
    wr32(CSR_BASE + CSR_DEPTH_MULT, 0x4);
    wr32(CSR_BASE + CSR_I_START,    0x10);
    wr32(CSR_BASE + CSR_I_END,      0x1F);
    wr32(CSR_BASE + CSR_W_START,    0x20);
    wr32(CSR_BASE + CSR_W_END,      0x2F);
    wr32(CSR_BASE + CSR_QUANT_SH,   0x7);

    // Read back and verify
    // conv_mode: only bit[0] is stored
    check("conv_mode",   rd32(CSR_BASE + CSR_CONV_MODE),  0x1);
    // p_mode: only bits[1:0]
    check("p_mode",      rd32(CSR_BASE + CSR_P_MODE),     0x2);
    check("i_size",      rd32(CSR_BASE + CSR_I_SIZE),     0xAA);
    check("i_c_size",    rd32(CSR_BASE + CSR_I_C_SIZE),   0xBB);
    check("o_c_size",    rd32(CSR_BASE + CSR_O_C_SIZE),   0xCC);
    check("o_size",      rd32(CSR_BASE + CSR_O_SIZE),     0xDD);
    check("stride",      rd32(CSR_BASE + CSR_STRIDE),     0x3);
    check("depth_mult",  rd32(CSR_BASE + CSR_DEPTH_MULT), 0x4);
    check("i_start_addr",rd32(CSR_BASE + CSR_I_START),    0x10);
    check("i_addr_end",  rd32(CSR_BASE + CSR_I_END),      0x1F);
    check("w_start_addr",rd32(CSR_BASE + CSR_W_START),    0x20);
    check("w_addr_end",  rd32(CSR_BASE + CSR_W_END),      0x2F);
    // quant_sh: SHIFT_WIDTH=8, only 8 bits stored
    check("quant_sh",    rd32(CSR_BASE + CSR_QUANT_SH),   0x7);

    // CTRL is write-only (auto-clears), always reads 0
    check("ctrl(write-only)", rd32(CSR_BASE + CSR_CTRL),  0x0);

    // STATUS: o_done should be 0 before we start anything
    uint32_t status = rd32(CSR_BASE + CSR_STATUS);
    printf("  [INFO] STATUS(o_done) = %d (expect 0 before start)\n", status & 1);

    // Flag registers not yet in RTL
    uint32_t sentinel = rd32(CSR_BASE + 0x3C);
    printf("  [INFO] 0x3C(quant_mult) = 0x%08x %s\n", sentinel,
           sentinel == 0xB0BACAFE ? "<NOT IN RTL YET>" : "");
    sentinel = rd32(CSR_BASE + 0x44);
    printf("  [INFO] 0x44(zero_point) = 0x%08x %s\n", sentinel,
           sentinel == 0xB0BACAFE ? "<NOT IN RTL YET>" : "");

    printf("Phase 1: %d passed, %d failed\n", pass_count, fail_count);

    // Reset CSR to clean state for actual run
    wr32(CSR_BASE + CSR_CTRL, 0x2);  // reg_clear pulse
}

// PHASE 2: DMA to SPAD load 
static void phase2_dma_load(void) {
    printf("\n=== PHASE 2: DMA Load SPADs ===\n");

    // Prepare test weight data in DDR
    // Weight SPAD: 1 weight = 0x01, packed into 64-bit SPAD words
    // Each 64-bit word holds 8 int8 weights; one DMA 32-bit beat = 4 bytes
    volatile uint32_t *wbuf = (volatile uint32_t*)DDR_WEIGHT_SRC;
    wbuf[0] = 0x01010101;   // 4 weights = all 1s
    wbuf[1] = 0x01010101;   // 4 weights = all 1s (completes one 64-bit SPAD word)

    // Prepare test ifmap data in DDR
    // Ifmap: 4 pixels [1,2,3,4] packed as int8 in 64-bit words
    volatile uint32_t *ibuf = (volatile uint32_t*)DDR_IFMAP_SRC;
    ibuf[0] = 0x04030201;   // pixels 1,2,3,4 in bytes
    ibuf[1] = 0x00000000;   // pad to full 64-bit word

    // CH0: load weights (DDR to weight SPAD)
    wr32(DMA_SPAD(CH_WEIGHTS), SPAD_WEIGHTS);
    wr32(DMA_SRC(CH_WEIGHTS),  DDR_WEIGHT_SRC);
    wr32(DMA_DST(CH_WEIGHTS),  SPAD_WRITE_ADAPTER);  // SPAD write adapter — M_SPAD_SEL routes to weights
    wr32(DMA_LEN(CH_WEIGHTS),  8);            // 8 bytes = one 64-bit SPAD word
    wr32(DMA_CTRL(CH_WEIGHTS), 0x1);
    printf("  Loading weights SPAD...\n");
    dma_wait(CH_WEIGHTS);
    printf("  [PASS] Weights DMA done (STATUS=DONE)\n");

    // CH1: load ifmaps (DDR to ifmap SPAD)
    wr32(DMA_SPAD(CH_IFMAPS),  SPAD_IFMAPS);
    wr32(DMA_SRC(CH_IFMAPS),   DDR_IFMAP_SRC);
    wr32(DMA_DST(CH_IFMAPS),   SPAD_WRITE_ADAPTER);  // SPAD write adapter — M_SPAD_SEL routes to ifmaps
    wr32(DMA_LEN(CH_IFMAPS),   8);            // 8 bytes = one 64-bit SPAD word
    wr32(DMA_CTRL(CH_IFMAPS),  0x1);
    printf("  Loading ifmap SPAD...\n");
    dma_wait(CH_IFMAPS);
    printf("  [PASS] Ifmap DMA done (STATUS=DONE)\n");
}

// PHASE 3: Run systolic array
static void phase3_run_systolic(void) {
    printf("\n=== PHASE 3: Run Systolic Array ===\n");

    // Minimal 2x2 pointwise conv, 1 input ch, 1 output ch
    wr32(CSR_BASE + CSR_CONV_MODE,  0);   // PW
    wr32(CSR_BASE + CSR_P_MODE,     0);
    wr32(CSR_BASE + CSR_I_SIZE,     2);   // 2x2 input
    wr32(CSR_BASE + CSR_I_C_SIZE,   1);
    wr32(CSR_BASE + CSR_O_C_SIZE,   1);
    wr32(CSR_BASE + CSR_O_SIZE,     2);   // 2x2 output
    wr32(CSR_BASE + CSR_STRIDE,     1);
    wr32(CSR_BASE + CSR_DEPTH_MULT, 1);
    wr32(CSR_BASE + CSR_I_START,    0);
    wr32(CSR_BASE + CSR_I_END,      0);   // 1 SPAD word of ifmaps
    wr32(CSR_BASE + CSR_W_START,    0);
    wr32(CSR_BASE + CSR_W_END,      0);   // 1 SPAD word of weights
    wr32(CSR_BASE + CSR_QUANT_SH,   0);   // no shift for easy verification

    printf("  Starting systolic array (CTRL[0]=1)...\n");
    wr32(CSR_BASE + CSR_CTRL, 0x1);       // route_en pulse

    // Poll STATUS[0] = o_done
    uint32_t timeout = 1000000;
    while (!(rd32(CSR_BASE + CSR_STATUS) & 0x1) && --timeout)
        ;

    if (timeout == 0) {
        printf("  [FAIL] Systolic array TIMEOUT — o_done never asserted!\n");
        fail_count++;
    } else {
        printf("  [PASS] o_done asserted. Systolic compute complete.\n");
        pass_count++;
    }
}

// PHASE 4: Read output SPAD via adapter to DDR
static void phase4_read_output(void) {
    printf("\n=== PHASE 4: DMA Output SPAD to DDR ===\n");

    // 2x2 output, 1 channel = 4 pixels
    // Each output SPAD word = 64-bit = 2 AXI beats of 32-bit
    // 4 pixels packed in 1 SPAD word = 8 bytes
    uint32_t output_bytes = 8;

    wr32(DMA_SRC(CH_OUTPUT),  OUT_SPAD_BASE);
    wr32(DMA_DST(CH_OUTPUT),  DDR_RESULT);
    wr32(DMA_LEN(CH_OUTPUT),  output_bytes);
    wr32(DMA_CTRL(CH_OUTPUT), 0x1);

    printf("  DMA reading output SPAD (%d bytes) to 0x%08x...\n",
           output_bytes, DDR_RESULT);
    dma_wait(CH_OUTPUT);
    printf("  [PASS] Output DMA done.\n");
    pass_count++;

    // Read results as 32-bit words — exactly what the RISC-V sees in DDR.
    // Adapter wrote: res[0]=bits[31:0] of SPAD word, res[1]=bits[63:32].
    volatile uint32_t *res = (volatile uint32_t*)DDR_RESULT;
    uint32_t beats = output_bytes / 4;

    printf("\n  Raw DDR (32-bit beats):\n");
    for (uint32_t i = 0; i < beats; i++)
        printf("    res[%u] = 0x%08x\n", i, res[i]);

    printf("\n  Output pixels (int8 per byte):\n");
    for (uint32_t i = 0; i < beats; i++) {
        for (int b = 0; b < 4; b++) {
            int idx = i * 4 + b;
            uint8_t val = (uint8_t)((res[i] >> (b * 8)) & 0xFF);
            printf("    out[%d] = %d (0x%02x)\n", idx, (int8_t)val, val);
        }
    }
}

// main
int main(void) {
    printf("=== Systolic Array Full Pipeline Test ===\n");

    phase1_csr_verify();
    phase2_dma_load();
    phase3_run_systolic();
    phase4_read_output();

    printf("\n=== SUMMARY: %d passed, %d failed ===\n", pass_count, fail_count);
    return fail_count;
}