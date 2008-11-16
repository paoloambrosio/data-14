#ifndef FSINTERNAL_H_
#define FSINTERNAL_H_

/* used by the dispatcher and by user/kern communication */

#define MSG_TYPE_FS_CREATE 1
#define MSG_TYPE_FS_LINK   2
#define MSG_TYPE_FS_RENAME 3
#define MSG_TYPE_FS_DELETE 4
#define MSG_TYPE_FS_ATTR   5
#define MSG_TYPE_FS_DATA   6

#endif /*FSINTERNAL_H_*/
