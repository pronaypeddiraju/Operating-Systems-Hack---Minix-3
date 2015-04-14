/* This file is concerned with allocating and freeing arbitrary-size blocks of
  2  * physical memory on behalf of the FORK and EXEC system calls.  The key data
  3  * structure used is the hole table, which maintains a list of holes in memory.
  4  * It is kept sorted in order of increasing memory address. The addresses
  5  * it contains refer to physical memory, starting at absolute address 0
  6  * (i.e., they are not relative to the start of MM).  During system
  7  * initialization, that part of memory containing the interrupt vectors,
  8  * kernel, and MM are "allocated" to mark them as not available and to
  9  * remove them from the hole list.
 10  *
 11  * The entry points into this file are:
 12  *   alloc_mem: allocate a given sized chunk of memory
 13  *   free_mem:  release a previously allocated chunk of memory
 14  *   mem_init:  initialize the tables when MM start up
 15  *   max_hole:  returns the largest hole currently available
 16  */
 17
 18 #include "mm.h"
 19 #include <minix/com.h>
 20
 21 #define NR_HOLES         128    /* max # entries in hole table */
 22 #define NIL_HOLE (struct hole *) 0
 23
 24 PRIVATE struct hole {
 25   phys_clicks h_base;           /* where does the hole begin? */
 26   phys_clicks h_len;            /* how big is the hole? */
 27   struct hole *h_next;          /* pointer to next entry on the list */
 28 } hole[NR_HOLES];
 29
 30
 31 PRIVATE struct hole *hole_head; /* pointer to first hole */
 32 PRIVATE struct hole *free_slots;        /* ptr to list of unused table slots */
 33
 34 FORWARD _PROTOTYPE( void del_slot, (struct hole *prev_ptr, struct hole *hp) );
 35 FORWARD _PROTOTYPE( void merge, (struct hole *hp)                           );
 36
 37
 38 /*===========================================================================*
 39  *                              alloc_mem                                    *
 40  *===========================================================================*/
 41 PUBLIC phys_clicks alloc_mem(clicks)
 42 phys_clicks clicks;             /* amount of memory requested */
 43 {
 44 /* Allocate a block of memory from the free list using first fit. The block
 45  * consists of a sequence of contiguous bytes, whose length in clicks is
 46  * given by 'clicks'.  A pointer to the block is returned.  The block is
 47  * always on a click boundary.  This procedure is called when memory is
 48  * needed for FORK or EXEC.
 49  */
 50
 51   register struct hole *hp, *prev_ptr = 0;
 52   phys_clicks old_base;
 53
 54   hp = hole_head;
 55   while (hp != NIL_HOLE)
      {
// EDIT LINES BEGIN
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

//EDIT LINES END

 72   }
 73   return(NO_MEM);
 74 }
 75
 76
 77 /*===========================================================================*
 78  *                              free_mem                                     *
 79  *===========================================================================*/
 80 PUBLIC void free_mem(base, clicks)
 81 phys_clicks base;               /* base address of block to free */
 82 phys_clicks clicks;             /* number of clicks to free */
 83 {
 84 /* Return a block of free memory to the hole list.  The parameters tell where
 85  * the block starts in physical memory and how big it is.  The block is added
 86  * to the hole list.  If it is contiguous with an existing hole on either end,
 87  * it is merged with the hole or holes.
 88  */
 89
 90   register struct hole *hp, *new_ptr, *prev_ptr = 0;
 91
 92   if (clicks == 0) return;
 93   if ( (new_ptr = free_slots) == NIL_HOLE) panic("Hole table full", NO_NUM);
 94   new_ptr->h_base = base;
 95   new_ptr->h_len = clicks;
 96   free_slots = new_ptr->h_next;
 97   hp = hole_head;
 98
 99   /* If this block's address is numerically less than the lowest hole currently
100    * available, or if no holes are currently available, put this hole on the
101    * front of the hole list.
102    */
103   if (hp == NIL_HOLE || base <= hp->h_base) {
104         /* Block to be freed goes on front of the hole list. */
105         new_ptr->h_next = hp;
106         hole_head = new_ptr;
107         merge(new_ptr);
108         return;
109   }
110
111   /* Block to be returned does not go on front of hole list. */
112   while (hp != NIL_HOLE && base > hp->h_base) {
113         prev_ptr = hp;
114         hp = hp->h_next;
115   }
116
117   /* We found where it goes.  Insert block after 'prev_ptr'. */
118   new_ptr->h_next = prev_ptr->h_next;
119   prev_ptr->h_next = new_ptr;
120   merge(prev_ptr);              /* sequence is 'prev_ptr', 'new_ptr', 'hp' */
121 }
122
123
124 /*===========================================================================*
125  *                              del_slot                                     *
126  *===========================================================================*/
127 PRIVATE void del_slot(prev_ptr, hp)
128 register struct hole *prev_ptr; /* pointer to hole entry just ahead of 'hp' */
129 register struct hole *hp;       /* pointer to hole entry to be removed */
130 {
131 /* Remove an entry from the hole list.  This procedure is called when a
132  * request to allocate memory removes a hole in its entirety, thus reducing
133  * the numbers of holes in memory, and requiring the elimination of one
134  * entry in the hole list.
135  */
136
137   if (hp == hole_head)
138         hole_head = hp->h_next;
139   else
140         prev_ptr->h_next = hp->h_next;
141
142   hp->h_next = free_slots;
143   free_slots = hp;
144 }
145
146
147 /*===========================================================================*
148  *                              merge                                        *
149  *===========================================================================*/
150 PRIVATE void merge(hp)
151 register struct hole *hp;       /* ptr to hole to merge with its successors */
152 {
153 /* Check for contiguous holes and merge any found.  Contiguous holes can occur
154  * when a block of memory is freed, and it happens to abut another hole on
155  * either or both ends.  The pointer 'hp' points to the first of a series of
156  * three holes that can potentially all be merged together.
157  */
158
159   register struct hole *next_ptr;
160
161   /* If 'hp' points to the last hole, no merging is possible.  If it does not,
162    * try to absorb its successor into it and free the successor's table entry.
163    */
164   if ( (next_ptr = hp->h_next) == NIL_HOLE) return;
165   if (hp->h_base + hp->h_len == next_ptr->h_base) {
166         hp->h_len += next_ptr->h_len;   /* first one gets second one's mem */
167         del_slot(hp, next_ptr);
168   } else {
169         hp = next_ptr;
170   }
171
172   /* If 'hp' now points to the last hole, return; otherwise, try to absorb its
173    * successor into it.
174    */
175   if ( (next_ptr = hp->h_next) == NIL_HOLE) return;
176   if (hp->h_base + hp->h_len == next_ptr->h_base) {
177         hp->h_len += next_ptr->h_len;
178         del_slot(hp, next_ptr);
179   }
180 }
181
182
183 /*===========================================================================*
184  *                              max_hole                                     *
185  *===========================================================================*/
186 PUBLIC phys_clicks max_hole()
187 {
188 /* Scan the hole list and return the largest hole. */
189
190   register struct hole *hp;
191   register phys_clicks max;
192
193   hp = hole_head;
194   max = 0;
195   while (hp != NIL_HOLE) {
196         if (hp->h_len > max) max = hp->h_len;
197         hp = hp->h_next;
198   }
199   return(max);
200 }
201
202
203 /*===========================================================================*
204  *                              mem_init                                     *
205  *===========================================================================*/
206 PUBLIC void mem_init(total, free)
207 phys_clicks *total, *free;              /* memory size summaries */
208 {
209 /* Initialize hole lists.  There are two lists: 'hole_head' points to a linked
210  * list of all the holes (unused memory) in the system; 'free_slots' points to
211  * a linked list of table entries that are not in use.  Initially, the former
212  * list has one entry for each chunk of physical memory, and the second
213  * list links together the remaining table slots.  As memory becomes more
214  * fragmented in the course of time (i.e., the initial big holes break up into
215  * smaller holes), new table slots are needed to represent them.  These slots
216  * are taken from the list headed by 'free_slots'.
217  */
218
219   register struct hole *hp;
220   phys_clicks base;             /* base address of chunk */
221   phys_clicks size;             /* size of chunk */
222   message mess;
223
224   /* Put all holes on the free list. */
225   for (hp = &hole[0]; hp < &hole[NR_HOLES]; hp++) hp->h_next = hp + 1;
226   hole[NR_HOLES-1].h_next = NIL_HOLE;
227   hole_head = NIL_HOLE;
228   free_slots = &hole[0];
229
230   /* Ask the kernel for chunks of physical memory and allocate a hole for
231    * each of them.  The SYS_MEM call responds with the base and size of the
232    * next chunk and the total amount of memory.
233    */
234   *free = 0;
235   for (;;) {
236         mess.m_type = SYS_MEM;
237         if (sendrec(SYSTASK, &mess) != OK) panic("bad SYS_MEM?", NO_NUM);
238         base = mess.m1_i1;
239         size = mess.m1_i2;
240         if (size == 0) break;           /* no more? */
241
242         free_mem(base, size);
243         *total = mess.m1_i3;
244         *free += size;
245   }
246 }
