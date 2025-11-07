// adsp2100_cases.inc
// Full expanded switch cases for ADSP2100 disassembler (0x04 → 0xFF)

case 0x04:  // stack control
    if ((op & 0x00ffe0) == 0x000000) {
        if (op & 0x000010) buffer += sprintf(buffer, "POP PC ");
        if (op & 0x000008) buffer += sprintf(buffer, "POP LOOP ");
        if (op & 0x000004) buffer += sprintf(buffer, "POP CNTR ");
        if ((op & 0x000003) == 0x000002) buffer += sprintf(buffer, "PUSH STAT ");
        else if ((op & 0x000003) == 0x000003) buffer += sprintf(buffer, "POP STAT ");
    } else buffer += sprintf(buffer, "??? (%06X)", op);
    break;

case 0x05:  // saturate MR
    if ((op & 0x00ffff) == 0x000000) buffer += sprintf(buffer, "IF MV SAT MR");
    else buffer += sprintf(buffer, "??? (%06X)", op);
    break;

case 0x06:  // DIVS
    if ((op & 0x00e0ff) == 0x000000)
        buffer += sprintf(buffer, "DIVS %s,%s", alu_yop[(op >> 11) & 3], alu_xop[(op >> 8) & 7]);
    else buffer += sprintf(buffer, "??? (%06X)", op);
    break;

case 0x07:  // DIVQ
    if ((op & 0x00f8ff) == 0x001000)
        buffer += sprintf(buffer, "DIVQ %s", alu_xop[(op >> 8) & 7]);
    else buffer += sprintf(buffer, "??? (%06X)", op);
    break;

case 0x08:  // reserved
    buffer += sprintf(buffer, "??? (%06X)", op);
    break;

case 0x09:  // modify address register
    if ((op & 0x00ffe0) == 0x000000) {
        temp = (op >> 2) & 4;
        buffer += sprintf(buffer, "MODIFY (I%d,M%d)", temp + ((op >> 2) & 3), temp + (op & 3));
    } else buffer += sprintf(buffer, "??? (%06X)", op);
    break;

case 0x0A:  // conditional return
    if ((op & 0x00fff0) == 0x000000) {
        buffer += sprintf(buffer, condition[op & 15]);
        if (op & 0x000010) buffer += sprintf(buffer, "RTI");
        else buffer += sprintf(buffer, "RTS");
    } else buffer += sprintf(buffer, "??? (%06X)", op);
    break;

case 0x0B:  // conditional jump (indirect)
    if ((op & 0x00ff00) == 0x000000) {
        buffer += sprintf(buffer, condition[op & 15]);
        if (op & 0x000010) buffer += sprintf(buffer, "CALL (I%d)", 4 + ((op >> 6) & 3));
        else buffer += sprintf(buffer, "JUMP (I%d)", 4 + ((op >> 6) & 3));
    } else buffer += sprintf(buffer, "??? (%06X)", op);
    break;

case 0x0C:  // mode control
    buffer += sprintf(buffer, mode_change[(op >> 4) & 3], "SEC_REG");
    buffer += sprintf(buffer, mode_change[(op >> 6) & 3], "BIT_REV");
    buffer += sprintf(buffer, mode_change[(op >> 8) & 3], "AV_LATCH");
    buffer += sprintf(buffer, mode_change[(op >> 10) & 3], "AR_SAT");
    buffer += sprintf(buffer, mode_change[(op >> 12) & 3], "M_MODE");
    buffer += sprintf(buffer, mode_change[(op >> 14) & 3], "TIMER");
    buffer += sprintf(buffer, mode_change[(op >> 2) & 3], "G_MODE");
    break;

case 0x0D:  // internal data move
    if ((op & 0x00f000) == 0x000000)
        buffer += sprintf(buffer, "%s = %s", reg_grp[(op >> 10) & 3][(op >> 4) & 15], reg_grp[(op >> 8) & 3][op & 15]);
    else buffer += sprintf(buffer, "??? (%06X)", op);
    break;

case 0x0E:  // conditional shift
    if ((op & 0x0080f0) == 0x000000) {
        buffer += sprintf(buffer, condition[op & 15]);
        buffer += sprintf(buffer, shift_op[(op >> 11) & 15], shift_xop[(op >> 8) & 7]);
    } else buffer += sprintf(buffer, "??? (%06X)", op);
    break;

case 0x0F:  // shift immediate
    if ((op & 0x008000) == 0x000000)
        buffer += sprintf(buffer, shift_by_op[(op >> 11) & 15], shift_xop[(op >> 8) & 7], (signed char)op);
    else buffer += sprintf(buffer, "??? (%06X)", op);
    break;

case 0x10:  // shift + internal move
    if ((op & 0x008000) == 0x000000) {
        buffer += sprintf(buffer, shift_op[(op >> 11) & 15], shift_xop[(op >> 8) & 7]);
        buffer += sprintf(buffer, ", %s = %s", reg_grp[0][(op >> 4) & 15], reg_grp[0][op & 15]);
    } else buffer += sprintf(buffer, "??? (%06X)", op);
    break;

case 0x11: case 0x12: case 0x13:  // shift + memory access
    buffer += sprintf(buffer, shift_op[(op >> 11) & 15], shift_xop[(op >> 8) & 7]);
    temp = (op >> 14) & 4;
    if (op & 0x008000)
        buffer += sprintf(buffer, ", %s(I%d,M%d) = %s", (op >> 10) & 1 ? "PM" : "DM", 4 + ((op >> 2) & 3), 4 + (op & 3), reg_grp[0][(op >> 4) & 15]);
    else
        buffer += sprintf(buffer, ", %s = %s(I%d,M%d)", reg_grp[0][(op >> 4) & 15], (op >> 10) & 1 ? "PM" : "DM", 4 + ((op >> 2) & 3), 4 + (op & 3));
    break;

case 0x14: case 0x15: case 0x16: case 0x17:  // do until
    buffer += sprintf(buffer, "DO $%04X UNTIL %s", (op >> 4) & 0x3fff, do_condition[op & 15]);
    break;

case 0x18: case 0x19: case 0x1A: case 0x1B: case 0x1C: case 0x1D: case 0x1E: case 0x1F:  // conditional jump
    if (op & 0x040000)
        buffer += sprintf(buffer, "%sCALL $%04X", condition[op & 15], (op >> 4) & 0x3fff);
    else
        buffer += sprintf(buffer, "%sJUMP $%04X", condition[op & 15], (op >> 4) & 0x3fff);
    break;
    // 0x20 → 0x27 conditional ALU/MAC
    case 0x20: case 0x21: case 0x22: case 0x23:
    case 0x24: case 0x25: case 0x26: case 0x27:
        buffer += sprintf(buffer, condition[op & 15]);
        buffer += alumac(buffer, (op >> 18) & 1, op);
        break;

    // 0x28 → 0x2F ALU/MAC + internal data register move
    case 0x28: case 0x29: case 0x2A: case 0x2B:
    case 0x2C: case 0x2D: case 0x2E: case 0x2F:
        if ((op & 0x03e000) != 0) {
            buffer += alumac(buffer, (op >> 18) & 1, op);
            buffer += sprintf(buffer, ", ");
        }
        buffer += sprintf(buffer, "%s = %s", reg_grp[0][(op >> 4) & 15], reg_grp[0][op & 15]);
        break;

    // 0x30 → 0x3F load non-data register immediate
    case 0x30: case 0x31: case 0x32: case 0x33:
    case 0x34: case 0x35: case 0x36: case 0x37:
    case 0x38: case 0x39: case 0x3A: case 0x3B:
    case 0x3C: case 0x3D: case 0x3E: case 0x3F:
        buffer += sprintf(buffer, "%s = $%04X", reg_grp[(op >> 18) & 3][op & 15], (op >> 4) & 0x3fff);
        break;

    // 0x40 → 0x5F load data register immediate / ALU/MAC + program memory
    case 0x40: case 0x41: case 0x42: case 0x43:
    case 0x44: case 0x45: case 0x46: case 0x47:
    case 0x48: case 0x49: case 0x4A: case 0x4B:
    case 0x4C: case 0x4D: case 0x4E: case 0x4F:
        buffer += sprintf(buffer, "%s = $%04X", reg_grp[0][op & 15], (op >> 4) & 0xffff);
        break;

    case 0x50: case 0x51: case 0x52: case 0x53:
    case 0x54: case 0x55: case 0x56: case 0x57:
    case 0x58: case 0x59: case 0x5A: case 0x5B:
    case 0x5C: case 0x5D: case 0x5E: case 0x5F:
        if ((op & 0x03e000) != 0) {
            buffer += alumac(buffer, (op >> 18) & 1, op);
            buffer += sprintf(buffer, ", ");
        }
        if (op & 0x080000)
            buffer += sprintf(buffer, "PM(I%d,M%d) = %s", 4 + ((op >> 2) & 3), 4 + (op & 3), reg_grp[0][(op >> 4) & 15]);
        else
            buffer += sprintf(buffer, "%s = PM(I%d,M%d)", reg_grp[0][(op >> 4) & 15], 4 + ((op >> 2) & 3), 4 + (op & 3));
        break;

    // 0x60 → 0x7F ALU/MAC + data memory
    case 0x60: case 0x61: case 0x62: case 0x63: case 0x64: case 0x65: case 0x66: case 0x67:
    case 0x68: case 0x69: case 0x6A: case 0x6B: case 0x6C: case 0x6D: case 0x6E: case 0x6F:
    case 0x70: case 0x71: case 0x72: case 0x73: case 0x74: case 0x75: case 0x76: case 0x77:
    case 0x78: case 0x79: case 0x7A: case 0x7B: case 0x7C: case 0x7D: case 0x7E: case 0x7F:
        if ((op & 0x03e000) != 0) {
            buffer += alumac(buffer, (op >> 18) & 1, op);
            buffer += sprintf(buffer, ", ");
        }
        temp = (op >> 18) & 4;
        if (op & 0x080000)
            buffer += sprintf(buffer, "DM(I%d,M%d) = %s", temp + ((op >> 2) & 3), temp + (op & 3), reg_grp[0][(op >> 4) & 15]);
        else
            buffer += sprintf(buffer, "%s = DM(I%d,M%d)", reg_grp[0][(op >> 4) & 15], temp + ((op >> 2) & 3), temp + (op & 3));
        break;

    // 0x80 → 0x9F read/write data memory (immediate)
    case 0x80 ... 0x9F:
        if (op & 0x100000)
            buffer += sprintf(buffer, "DM($%04X) = %s", (op >> 4) & 0x3fff, reg_grp[(op >> 18) & 3][op & 15]);
        else
            buffer += sprintf(buffer, "%s = DM($%04X)", reg_grp[(op >> 18) & 3][op & 15], (op >> 4) & 0x3fff);
        break;

    // 0xA0 → 0xBF data memory write (immediate)
    case 0xA0 ... 0xBF:
        temp = (op >> 18) & 4;
        buffer += sprintf(buffer, "DM(I%d,M%d) = $%04X", temp + ((op >> 2) & 3), temp + (op & 3), (op >> 4) & 0xffff);
        break;

    // 0xC0 → 0xFF ALU/MAC with data & program read
    case 0xC0 ... 0xFF:
        if ((op & 0x03e000) != 0) {
            buffer += alumac(buffer, 0, op);
            buffer += sprintf(buffer, ", ");
        }
        buffer += sprintf(buffer, "%s = DM(I%d,M%d), %s = PM(I%d,M%d)",
                dual_xreg[(op >> 18) & 3], (op >> 2) & 3, op & 3,
                dual_yreg[(op >> 20) & 3], 4 + ((op >> 6) & 3), 4 + ((op >> 4) & 3));
        break;
