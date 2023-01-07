/**
 * estp-totp library
 * Source: https://github.com/huming2207/esp-totp
 *
 * Portions of estp-totp were derived from the "c_otp" library
 * Source: https://github.com/fmount/c_otp
 * 
 * MIT License
 * 
 * Copyright (c) 2019 Jackson Ming Hu <huming2207@gmail.com> 
 * Copyright (c) 2017 fmount <fmount9@autistici.org>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#pragma once

class otp
{
    public:
        static uint32_t hotp_generate(uint8_t *key, size_t key_len, uint64_t interval, size_t digits);
        static uint32_t totp_hash_token(uint8_t *key, size_t key_len, uint64_t time, size_t digits);
        static uint32_t totp_generate(uint8_t *key, size_t key_len);

    private:
        static void hotp_hmac(unsigned char *key, size_t ken_len, uint64_t interval, uint8_t *out);
        static uint32_t hotp_dt(const uint8_t *digest);
};

