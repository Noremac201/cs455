#include <stdio.h>
#include <stdlib.h>
#define BUFFER_SIZE 256

int main()
{
    unsigned char buffer[BUFFER_SIZE];
    FILE *cover_fptr;
    FILE *orig_fptr;
    int byte_r;
    cover_fptr = fopen("file.txt", "wb");
    orig_fptr = fopen("asdf.txt", "rb");

    if (orig_fptr)
    {
        while (byte_r = (fread(buffer, sizeof(unsigned char), BUFFER_SIZE, orig_fptr)))
        {
            for (int i=0; i < byte_r; i++)
            {
                unsigned char c = buffer[i];
                c = c ^ 0;
                buffer[i] = c;
            }
            printf("%s", buffer);
        }
    }

    fclose(orig_fptr);

    return 0;
}

