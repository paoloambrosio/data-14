CREATE TABLE revision (num INTEGER PRIMARY KEY AUTOINCREMENT, time INTEGER);

CREATE TABLE fs_inode (fsid INTEGER PRIMARY KEY, id INTEGER);

CREATE TABLE inode (id INTEGER PRIMARY KEY AUTOINCREMENT, type INTEGER,
	nlink INTEGER);

CREATE TABLE dentry (inode_id INTEGER INDEXED, parent_id INTEGER,
	v_start INTEGER, v_end INTEGER, name VARCHAR );

CREATE TABLE inode_attr (inode_id INTEGER INDEXED,
	v_start INTEGER, owner_id INTEGER, group_id INTEGER, security INTEGER);

CREATE TABLE inode_data (id INTEGER PRIMARY KEY AUTOINCREMENT,
	inode_id INTEGER INDEXED, v_start INTEGER);

