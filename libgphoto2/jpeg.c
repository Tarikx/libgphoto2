/* jpeg.c
 * This code was written by Nathan Stenzel for gphoto
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <stdio.h>
#include "jpeg.h"

// call example:nullpictureabort(picture,"Picture");
#define nullpointerabort(pointer,name) \
    if (pointer==NULL) { printf(name " does not exist\n"); \
        return; }


chunk *chunk_new(int length)
{
chunk *mychunk;
    mychunk =  (chunk *) malloc(sizeof(chunk));
    mychunk->size=length;
//    printf("New chunk of size %i\n", mychunk->size);
    mychunk->data = (char *)malloc(length);
    if (mychunk==NULL) printf("Failed to allocate new chunk!\n");
    return mychunk;
}

void chunk_print(chunk *mychunk)
{
    int x;
//    printf("Size=%i\n", mychunk->size);
    nullpointerabort(mychunk, "Chunk");
    for (x=0; x<mychunk->size; x++)
        printf("%hX ", mychunk->data[x]);
    printf("\n");
}

void chunk_destroy(chunk *mychunk)
{
    nullpointerabort(mychunk, "Chunk");
    mychunk->size=0;
    free(mychunk->data);
    free(mychunk);
}

void jpeg_init(jpeg *myjpeg)
{
    myjpeg->count=0;
}

void jpeg_destroy(jpeg *myjpeg)
{
    int count;
    for (count=0; count<myjpeg->count; count++)
        chunk_destroy(myjpeg->marker[count]);
    myjpeg->count=0;
}

char jpeg_findff(int *location, chunk *picture) {
//printf("Entered jpeg_findamarker!!!!!!\n");
    nullpointerabort(picture, "Picture");
    while(*location<picture->size)
    {
//        printf("%hX ",picture->data[*location]);
        if (picture->data[*location]==0xff)
        {
//        printf("return success!\n");
        return 1;
        }
    (*location)++;
    }
//    printf("return failure!\n");
    return 0;
}

char jpeg_findactivemarker(char *id, int *location, chunk *picture) {
//    printf("Entered jpeg_findactivemarker!!!!!!!!\n");
    nullpointerabort(picture, "Picture");
    while(jpeg_findff(location, picture) && ((*location+1)<picture->size))
        if (picture->data[*location+1]) {
            *id=picture->data[*location+1];
            return 1;
        }
    return 0;
}

void jpeg_add_marker(jpeg *myjpeg, chunk *picture, int start, int end)
{
    int length;
    nullpointerabort(picture, "Picture");
    length=(int)(end-start+1);
//    printf("Add marker #%i starting from %i and ending at %i for a length of %i\n", myjpeg->count, start, end, length);
    myjpeg->marker[myjpeg->count] = chunk_new(length);
//    printf("Read length is: %i\n", myjpeg->marker[myjpeg->count].size);
    memcpy(myjpeg->marker[myjpeg->count]->data, picture->data+start, length);
    chunk_print(myjpeg->marker[myjpeg->count]);
    myjpeg->count++;
}

char *jpeg_markername(int c)
{
    int x;
//    printf("searching for marker %X in list\n",c);
//    printf("%i\n", sizeof(markers));
    for (x=0; x<sizeof(JPEG_MARKERS); x++)
    {
//        printf("checking to see if it is marker %X\n", markers[x]);

        if (c==JPEG_MARKERS[x])
            return (char *)JPEG_MARKERNAMES[x];
    }
    return "Undefined marker";
}

void jpeg_parse(jpeg *myjpeg, chunk *picture)
{
    int position=0;
    int lastposition;
    char id;
    nullpointerabort(picture,"Picture");
    if (picture->data[0]!=0xff)
        {
            jpeg_findactivemarker(&id, &position, picture);
            jpeg_add_marker(myjpeg,picture,0,position-1);
            lastposition=position;
            position=position++;
        }
        else
        {
        lastposition=position;
        position+=2;
        jpeg_findactivemarker(&id, &position, picture);
        jpeg_add_marker(myjpeg,picture,0,position-1);
        lastposition=position;
        position+=2;
        }
    while (position<picture->size)
        {
            jpeg_findactivemarker(&id, &position, picture);
            jpeg_add_marker(myjpeg,picture,lastposition,position-1);
            lastposition=position;
            position+=2;
        }
    position-=2;
    if (position<picture->size)
    jpeg_add_marker(myjpeg,picture,lastposition,picture->size-1);
}

void jpeg_print(jpeg *myjpeg)
{
    int c;
    printf("There are %i markers\n", myjpeg->count);
    for (c=0; c < myjpeg->count; c++)
    {
        printf("%s:\n",jpeg_markername(myjpeg->marker[c]->data[1]));
        chunk_print(myjpeg->marker[c]);
    }
}

chunk * jpeg_make_SOFC (int width, int height, char vh1, char vh2, char vh3)
{
    chunk *target;

    char sofc_data[]={
        0xFF, 0xC0, 0x00, 0x11,  0x08, 0x00, 0x80, 0x01,
        0x40, 0x03, 0x01, 0x11,  0x00, 0x02, 0x21, 0x01,  0x03, 0x11, 0x00
        };
    target = chunk_new(sizeof(sofc_data));
    nullpointerabort(target, "New SOFC");
    memcpy(target->data, sofc_data, sizeof(sofc_data));
    target->data[5] = (height&0xff00) >> 8;
    target->data[6] = height&0xff;
    target->data[7] = (width&0xff00) >> 8;
    target->data[8] = width&0xff;
    target->data[11]= vh1;
    target->data[14]= vh2;
    target->data[17]= vh3;
    return target;
}

void jpeg_print_quantization_table(jpeg_quantization_table *table)
{
    int x;
    nullpointerabort(table, "Quantization table");
    for (x=0; x<64; x++)
    {
        if (x && ((x%8)==0))
        {
            printf("\n");
        }
        printf("%3i ", (*table)[x]);
    }
    printf("\n");
}

#define TESTING_JPEG_C

#ifdef TESTING_JPEG_C
/* TEST CODE SECTION */
char testdata[] ={
    0xFF,0xD8, 0xFF,0xE0, 0xff,0xDB, 0xFF,0xC4, 0xFF,0xDA,
    0xFF,0xC0, 0xff,0xff};

jpeg_quantization_table mytable = {
      2,  3,  4,  5,  6,  7,  8,  9,
      3,  4,  5,  6,  7,  8,  9, 10,
      4,  5,  6,  7,  8,  9, 10, 11,
      5,  6,  7,  8,  9, 10, 11, 12,
      6,  7,  8,  9, 10, 11, 12, 13,
      7,  8,  9, 10, 11, 12, 13, 14,
      8,  9, 10, 11, 12, 13, 14, 15,
      9, 10, 11, 12, 13, 14, 15, 16};

int main()
{
chunk *picture;
jpeg myjpeg;
char id;
int location=0,x;
printf("Print the test quantization table\n");
jpeg_print_quantization_table(&mytable);
jpeg_init(&myjpeg);
picture = chunk_new(sizeof(testdata));
picture->data=testdata;
picture->size=sizeof(testdata);
printf("testdata size is %i\n",picture->size);
chunk_print(picture);
printf("Call jpeg_parse!!!!!!!!!!!!!!!!!!!!!!!\n");
jpeg_parse(&myjpeg,picture);

printf("\nPrint the jpeg table\n");
jpeg_print(&myjpeg);
printf("\nCall jpeg_destroy\n");
jpeg_destroy(&myjpeg);
/* You can't call chunk_destroy because testdata is a constant.
 * Since picture->data points to it, it would segfault.
 */
free(picture);
printf("chunk_new and chunk_destroy tests\n");
picture = chunk_new(10);
for (x=0; x<10; x++) picture->data[x]=x;
for (x=0; x<10; x++) printf("%hX ",picture->data[x]);
chunk_destroy(picture);
printf("\n");
}
#endif
