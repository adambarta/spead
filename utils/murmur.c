uint32_t Murmur3_32(const char* key, uint32_t len, uint32_t seed)
{
        static const uint32_t c1 = 0xcc9e2d51;
        static const uint32_t c2 = 0x1b873593;
        static const uint32_t r1 = 15;
        static const uint32_t r2 = 13;
        static const uint32_t m = 5;
        static const uint32_t n = 0xe6546b64;
        
        uint32_t hash = seed;
        
        uint32_t* keydata = (uint32_t*) key; //used to extract 32 bits at a time
        int keydata_it = 0;
        
        while (len >= 4)
        {
                uint32_t k = keydata[keydata_it++];
                len -= 4;
                
                k *= c1;
                k = (k << r1) | (k >> (32-r1));
                k *= c2;
                
                hash ^= k;
                hash = ((hash << r2) | (hash >> (32-r2)) * m) + n;
        }

        const uint8_t * tail = (const uint8_t*)(keydata + keydata_it*4);
        uint32_t k1 = 0;

        switch(len & 3) {
        case 3:
                k1 ^= tail[2] << 16;
        case 2:
                k1 ^= tail[1] << 8;
        case 1:
                k1 ^= tail[0];

                k1 *= c1;
                k1 = (k1 << r1) | (k1 >> (32-r1));
                k1 *= c2;
                hash ^= k1;
        }
        
        hash ^= len;
        hash ^= (hash >> 16);
        hash *= 0x85ebca6b;
        hash ^= (hash >> 13);
        hash *= 0xc2b2ae35;
        hash ^= (hash >> 16);
        
        return hash;
}
