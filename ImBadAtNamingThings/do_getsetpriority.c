int do_getsetpriority()
 {
         int r, arg_which, arg_who, arg_pri;
         struct mproc *rmp;

         arg_which = m_in.m_lc_pm_priority.which;
         arg_who = m_in.m_lc_pm_priority.who;
         arg_pri = m_in.m_lc_pm_priority.prio;   /* for SETPRIORITY */

         /* Code common to GETPRIORITY and SETPRIORITY. */

         /* Only support PRIO_PROCESS for now. */
         if (arg_which != PRIO_PROCESS)
                 return(EINVAL);

         if (arg_who == 0)
                rmp = mp;
         else
                 if ((rmp = find_proc(arg_who)) == NULL)
                         return(ESRCH);

         if (mp->mp_effuid != SUPER_USER &&
            mp->mp_effuid != rmp->mp_effuid && mp->mp_effuid != rmp->mp_realuid)
                 return EPERM;

         /* If GET, that's it. */
         if (call_nr == PM_GETPRIORITY) {
                 return(rmp->mp_nice - PRIO_MIN);
         }

         /* Only root is allowed to reduce the nice level. */
         if (rmp->mp_nice > arg_pri && mp->mp_effuid != SUPER_USER)
                 return(EACCES);

         /* We're SET, and it's allowed.
          *
          * The value passed in is currently between PRIO_MIN and PRIO_MAX.
          * We have to scale this between MIN_USER_Q and MAX_USER_Q to match
          * the kernel's scheduling queues.
          */

         if ((r = sched_nice(rmp, arg_pri)) != OK) {
                 return r;
         }

         rmp->mp_nice = arg_pri;
         return(OK);
}
