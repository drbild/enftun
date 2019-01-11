/*
 * Copyright 2018-2019 Xaptum, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "ip.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <arpa/inet.h>

const struct in6_addr ip6_all_nodes = {
    .s6_addr = { // ff02::1
        0xFF, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01
    }
};

const struct in6_addr ip6_all_routers = {
    .s6_addr = { // fe02::2
        0xFF, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02
    }
};

const struct in6_addr ip6_default = {
    .s6_addr = { // ::
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    }
};

const struct in6_addr ip6_self = {
    .s6_addr = { // fe80::1
        0xfe, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01
    }
};

int ip6_prefix_str(const struct in6_addr* addr,
                   const int prefix, char* dst,
                   size_t size)
{
    int rc;

    /* Print the IP address */
    if (inet_ntop(AF_INET6, addr, dst, size) == NULL)
        return -1;

    int len = strlen(dst);
    size -= len; dst += len;

    /* Print the slash */
    if (size < 2)
        return -1;

    dst[0] = '/';
    dst[1] = '\0';
    size -= 1; dst += 1;

    /* Print the prefix */
    rc = snprintf(dst, size, "%d", prefix);
    if ((rc < 0) || ((size_t) rc >= size))
        return -1;

    return 0;
}

#if 1
int
ip6_prefix(const char* str,
           struct in6_addr* prefix,
           uint8_t* prefixlen)
{
    if (0 == strcmp(str, "default"))
    {
        *prefix = ip6_default;
        *prefixlen = 0;
        return 0;
    }

    char buf[64];
    strncpy(buf, str, sizeof(buf));

    char *slash = strchr(buf, '/');
    if ( slash ) {
        *slash = 0;
        *prefixlen = atoi(slash+1);

        if ( 128 < *prefixlen )
            return -1;

        return 1 == inet_pton(AF_INET6, buf, prefix) ? 0 : -1;
    }

    return -1;
}
#else
int
ip6_prefix(const char* s,
           struct in6_addr* prefix,
           uint8_t* prefixlen)
{
    int field = 0;
    uint16_t field_value = 0;

    while(field < 8) {
        switch(*s) {
        case '/':
            if ( field != 7 )
                goto error;

            prefix->s6_addr16[field] = htons(field_value);

            // Get prefix length
            s++;
            *prefixlen = 0;

            while (*s) {
                switch(*s) {
                case '0':
                case '1':
                case '2':
                case '3':
                case '4':
                case '5':
                case '6':
                case '7':
                case '8':
                case '9':
                    *prefixlen = 10*(*prefixlen) + *s - '0';
                    break;
                default:
                    goto error;
                }

                s++;
            }

            if ( 128 < *prefixlen )
                goto error;

            return 0;
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
            field_value = (field_value << 4) | (*s - '0');
            break;
        case 'a':
        case 'b':
        case 'c':
        case 'd':
        case 'e':
        case 'f':
            field_value = (field_value << 4) | (10 + *s - 'a');
            break;
        case 'A':
        case 'B':
        case 'C':
        case 'D':
        case 'E':
        case 'F':
            field_value = (field_value << 4) | (10 + *s - 'A');
            break;
        case ':':
            prefix->s6_addr16[field++] = htons(field_value);
            field_value = 0;

            if ( ':' == s[1] ) {
                // double colon found, this can only happen once
                // is indication that at least one 0 is missing

                s++;
                prefix->s6_addr16[field++] = 0;

                // more zeros may be missing
                int remaining_fields = 1;
                for (const char *p = s+1; *p; ++p)
                    if ( *p == ':' )
                        remaining_fields++;

                while (8 > field + remaining_fields)
                    prefix->s6_addr16[field++] = 0;
            }
            break;
        default:
            // Invalid character found in address
            goto error;
        }

        s++;
    }

error:

    return -1;
}
#endif
