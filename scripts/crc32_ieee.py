# CRC IEEE algorithm

"""c
uint32_t crc32_ieee_u8(uint8_t *buf, size_t len) {
	uint32_t crc = 0xffffffff;

	for (uint8_t *b = buf; b < buf + len; b++) {
		crc = (crc << 8) ^ crc32_ieee_table[((crc >> 24) ^ *b) & 0xff];
	}

	return crc;
}

uint32_t crc32_ieee_u32(uint32_t *buf, size_t len) {
	uint32_t crc = 0xffffffff;

	for (uint32_t *b = buf; b < buf + len; b++) {
		crc = (crc << 8) ^ crc32_ieee_table[((crc >> 24) ^ ((*b) >> 24)) & 0xff];
		crc = (crc << 8) ^ crc32_ieee_table[((crc >> 24) ^ ((*b) >> 16)) & 0xff];
		crc = (crc << 8) ^ crc32_ieee_table[((crc >> 24) ^ ((*b) >> 8)) & 0xff];
		crc = (crc << 8) ^ crc32_ieee_table[((crc >> 24) ^ ((*b) >> 0)) & 0xff];
	}

	return crc;
}
"""

def int_to_bytes(i):
    return [(i >> 24) & 0xFF, (i >> 16) & 0xFF, (i >> 8) & 0xFF, i & 0xFF]

def to_u32_c_array(table):
    s = "static const uint32_t crc32_ieee_table[] = {"
    for i in range(len(table)):
        if i & 0x3 == 0:
            s += "\n\t"
        s += f"0x{table[i]:08x}U, "
    s += "\n};"
    return s

def gen_table(poly):
    table = list()

    for i in range(0x100):
        c = i << 24

        for _ in range(8):
            c = (c << 1) ^ poly if (c & 0x80000000) else c << 1

        table.append(c & 0xffffffff)

    return table

def crc32(buf, table):
    crc = 0xffffffff

    for integer in buf:
        b = int_to_bytes(integer)

        for byte in b:
            crc = ((crc << 8) & 0xffffffff) ^ table[(crc >> 24) ^ byte]

    return crc

if __name__ == "__main__":
    buf = [0, 1] # u32
    crc32_ieee_poly = 0x04C11DB7
    table = gen_table(crc32_ieee_poly)
    result = crc32(buf, table)
    print("crc " + hex(result))

    print(to_u32_c_array(table))