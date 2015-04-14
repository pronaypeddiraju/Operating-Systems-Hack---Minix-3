#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <lib.h>


int main(int argc, char *argv[])
{

   do
   {
        prev_ptr = NIL_HOLE;
        hp = hole_head;
        candidate = hole_head;

        last_hole_prev = NIL_HOLE;
        while(hp != NIL_HOLE && hp->h_base < swap_base)
        {
            if(hp->h_len >= clicks && hp->h_len < candidate->h_len)
            {
                last_hole_prev = prev_ptr;
                candidate = hp;
            }
            prev_ptr = last_hole_prev;
            hp = candidate;
            last_hole_prev = NIL_HOLE;
        }
        if (candidate != NIL_HOLE)
        {
            hp = candidate; prev_ptr = last_hole_prev;
            /* We found a hole that is big enough.  Use it. */
      		old_base = hp->h_base;	/* remember where it started */
      		hp->h_base += clicks;	/* bite a piece off */
      		hp->h_len -= clicks;	/* ditto */

      		// Remember new high watermark of used memory.
      		if(hp->h_base > high_watermark)
      		high_watermark = hp->h_base;

      		// Delete the hole if used up completely.
      		if (hp->h_len == 0) del_slot(prev_ptr, hp);

      		// Return the start address of the acquired block.
      		return(old_base);
         }

    }while(swap_out());     //try to swap out some other process
return(NO_MEM);
}
