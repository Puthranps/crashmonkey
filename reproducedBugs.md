# Bugs reproduced by CrashMonkey and Ace


#### Summary:
	Total unique bugs reproduced = 26
		1. Fails on btrfs = 24
		2. Fails on f2fs  = 2
		3. Fails on ext4  = 2
		4. Fails on xfs   = 0


The workloads to reproduce the following bugs can be found [here](https://github.com/utsaslab/crashmonkey/tree/master/code/tests).

___



1. ### generic_034 ###
	If a directory entry is both found in the persisted metadata and in the fsync log, at log replay time the directory entry should update the right value of i_size for the files created before a crash. After recovery, removing all the files in this directory should ensure that the directory is removable.

	**Result** : Fails on btrfs (kernel 3.13). [The directory is unremovable due to incorrect i_size.](https://patchwork.kernel.org/patch/4864571/)

	**Output** :
	```
	Reordering tests ran 64 tests with
		passed cleanly: 22
		passed fixed: 0
		fsck required: 0
		failed: 42
			old file persisted: 0
			file missing: 0
			file data corrupted: 0
			file metadata corrupted: 42
			other: 0

	```

2. ### generic_039 ###
	If we decrement link count for a file, fsync and crash, on replaying the log tree, the i_size due to the removed link should be decremented. Also there should be no dangling directory index references that makes the directory unremovable.

	**Result** : Fails on btrfs (kernel 3.13). [The parent directory is unremovable due to incorrect i_size.](https://patchwork.kernel.org/patch/6058101/)

	**Output** :
	```
	Reordering tests ran 476 tests with
		passed cleanly: 213
		passed fixed: 0
		fsck required: 0
		failed: 263
			old file persisted: 0
			file missing: 0
			file data corrupted: 0
			file metadata corrupted: 263
			other: 0

	```

3. ### generic_041 ###
	Consider an inode with a large number of hard links, some of which may be extrefs. If we turn a regular ref into an extref, fsync the inode and then crash, the filesystem should be consistent and mountable.

	**Result** : Fails on btrfs (kernel 3.13). Fsync log makes the replay code always fail with -EOVERFLOW when processing the inode's references. [Makes the filesystem unmountable.](https://www.spinics.net/lists/linux-btrfs/msg41157.html)

	**Output** :
	```
	Reordering tests ran 278 tests with
		passed cleanly: 102
		passed fixed: 0
		fsck required: 176
		failed: 0
			old file persisted: 0
			file missing: 0
			file data corrupted: 0
			file metadata corrupted: 0
			other: 0

	```

4. ### generic_056 ###
	If we write data into a file and fsync it, and later create a hard link to the same file, and persist the fsync log, on recovery the file data is lost.

	**Result** : Fails on btrfs (kernel 3.13). The file that was previously written to and fsynced, loses data adding a hard link and persisting the fsync log- because the i_size is updated incorrectly during fsync log replay. [Data inconsistency](https://patchwork.kernel.org/patch/5822681)

	**Output** :
	```
	Reordering tests ran 24 tests with
		passed cleanly: 15
		passed fixed: 0
		fsck required: 0
		failed: 9
			old file persisted: 0
			file missing: 0
			file data corrupted: 9
			file metadata corrupted: 0
			incorrect block count: 0
			other: 0

	```


5. ### generic_059 ###
	If we punch a hole for a small range (partial page), and fsync the file, the operation should persist.

	**Result** : Fails on btrfs (kernel 3.13). [Inode item was not updated, making fsync a no-op.](https://patchwork.kernel.org/patch/5830801/)

	**Output** :
	```
	Reordering tests ran 680 tests with
		passed cleanly: 561
		passed fixed: 0
		fsck required: 0
		failed: 119
			old file persisted: 0
			file missing: 0
			file data corrupted: 119
			file metadata corrupted: 0
			other: 0

	```

6. ### generic_066 ###
	Test that if we delete a xattr from a file and fsync the file, after log replay the file should not have the deleted xattr.

	**Result** : Fails on btrfs (kernel 3.13). [The deleted xattr appears in the file, even after fsync.](https://www.spinics.net/lists/linux-btrfs/msg42162.html)

	**Output** :
	```
	Reordering tests ran 28 tests with
		passed cleanly: 24
		passed fixed: 0
		fsck required: 0
		failed: 4
			old file persisted: 0
			file missing: 0
			file data corrupted: 0
			file metadata corrupted: 4
			other: 0

	```


7. ### generic_090 ###
	If we append data to a file and fsync it, and previously create a hard link to the same file in a different transaction, the new i_size is not logged due to the fsync, thereby losing the appended data.

	**Result** : Fails on btrfs (kernel 3.12). In the append write path, fsync didnot log the inode item in a new transaction. [Data inconsistency](https://patchwork.kernel.org/patch/6624481)

    **Output** :
	```
	Reordering tests ran 1121 tests with
		passed cleanly: 1120
		passed fixed: 0
		fsck required: 0
		failed: 1
			old file persisted: 0
			file missing: 0
			file data corrupted: 1
			file metadata corrupted: 0
			incorrect block count: 0
			other: 0        
	```


8. ### generic_104 ###
    Test that if we create hard link for two files within the same directory, but fsync only one of them, on replay the directory must be removable after unlinking the original and linked files.

    **Result** : Fails on btrfs (kernel 4.1.1). [The directory becomes unremovable even after deleting all files within.](https://patchwork.kernel.org/patch/6852751/)

    **Output** :
    ```
	Reordering tests ran 272 tests with
		passed cleanly: 18
		passed fixed: 0
		fsck required: 0
		failed: 254
			old file persisted: 0
			file missing: 0
			file data corrupted: 0
			file metadata corrupted: 254
			incorrect block count: 0
			other: 0

    ```


9. ### generic_106 ###
	Test that if we remove a hard link for an inode, evict the inode, fsync it and crash, on replay the directory must be removable after unlinking the original file.

	**Result** : Fails on btrfs (kernel 3.13). [The directory becomes unremovable even after deleting all files within.](https://patchwork.kernel.org/patch/6860971)

	**Output** :
	```
	Reordering tests ran 25 tests with
		passed cleanly: 16
		passed fixed: 0
		fsck required: 0
		failed: 9
			old file persisted: 0
			file missing: 0
			file data corrupted: 0
			file metadata corrupted: 9
			incorrect block count: 0
			other: 0

	```

10. ### generic_107 ###
	Test that if we remove a hard link in a different parent directory from an inode, evict the inode, fsync it and crash, on replay the directory must be removable after unlinking the original file.  

	**Result** : Fails on btrfs (kernel 3.13). [The directory becomes unremovable even after deleting all files within.](https://www.spinics.net/lists/linux-btrfs/msg45915.html)

	**Output** :
	```
	Reordering tests ran 25 tests with
		passed cleanly: 16
		passed fixed: 0
		fsck required: 0
		failed: 9
			old file persisted: 0
			file missing: 0
			file data corrupted: 0
			file metadata corrupted: 9
			incorrect block count: 0
			other: 0

	```


11. ### generic_177 ###
    Test that if we punch a hole on overalapping regions and fsync the file, the file layout and extent map is persisted after a crash.

    **Result** : Fails on btrfs when using the no-holes feature (kernel 4.1.1). [Extent map incorrect after recovery.](https://patchwork.kernel.org/patch/7536021/)

    **Output** :
    ```
	Reordering tests ran 343 tests with
		passed cleanly: 342
		passed fixed: 0
		fsck required: 0
		failed: 1
			old file persisted: 0
			file missing: 0
			file data corrupted: 1
			file metadata corrupted: 0
			incorrect block count: 0
			other: 0
    ```


12. ### generic_321 ###
    Test that if we rename a file from one directory to another, and fsync both the destination directory as well as the renamed file, the rename should persist. Also, this file should be removable, and allow the destination directory to be removed as well.

    **Result** : Fails on btrfs (kernel 3.12). [The renamed file is persisted in both directories and becomes unremovable from destination.](https://patchwork.kernel.org/patch/3234531/)

    **Output** :
    ```
    Reordering tests ran 83 tests with
        passed cleanly: 18
        passed fixed: 0
        fsck required: 0
        failed: 65
                old file persisted: 65
                file missing: 0
                file data corrupted: 0
                file metadata corrupted: 0
                incorrect block count: 0
                other: 0

    ```

13. ### generic_322 ###
    Test that if we write to a file, rename it within the same directory, and fsync the renamed file, the rename should persist.

    **Result** : Fails on btrfs (kernel 3.12). [The renamed file didnot survive the fsync](https://patchwork.kernel.org/patch/3234541/)

    **Output** :
    ```
	Reordering tests ran 83 tests with
		passed cleanly: 82
		passed fixed: 0
		fsck required: 0
		failed: 1
			old file persisted: 0
			file missing: 1
			file data corrupted: 0
			file metadata corrupted: 0
			incorrect block count: 0
			other: 0

    ```

14. ### generic_325 ###
    Test that if we do a ranged fsync(msync a range covering partial dirty pages), followed by a ranged fsync of the remaining dirty pages, then after a crash the data must be preserved.

    **Result** : Fails on btrfs (kernel 3.16). The second msync was a no-op in btrfs [Data inconsistency](https://patchwork.kernel.org/patch/4813651/)

    **Output** :
    ```
	Reordering tests ran 8354 tests with
		passed cleanly: 8353
		passed fixed: 0
		fsck required: 0
		failed: 1
			old file persisted: 0
			file missing: 0
			file data corrupted: 1
			file metadata corrupted: 0
			incorrect block count: 0
			other: 0

    ```


15. ### generic_335 ###
	Suppose we move a file from one directory to another and fsync the parent of the old directory. If we crash and remount the filesystem, file that we moved must exist and be present in the new directory.


	**Result** : Fails on btrfs (kernel 3.13). [The file that we moved is lost and mv is unsuccessful.](https://patchwork.kernel.org/patch/8312681/)

	**Output** :
	```
	Reordering tests ran 11 tests with
		passed cleanly: 10
		passed fixed: 0
		fsck required: 0
		failed: 1
			old file persisted: 0
			file missing: 1
			file data corrupted: 0
			file metadata corrupted: 0
			incorrect block count: 0
			other: 0

	```

16. ### generic_336 ###
	Suppose we move a file from one directory to another and fsync the file. Before this, the directory inode containing this file must have been fsynced/logged. If we crash and remount the filesystem, file that we moved must exist


	**Result** : Fails on btrfs (kernel 4.4). [The file that we moved is lost.](https://patchwork.kernel.org/patch/8293181/)

	**Output** :
	```
	Reordering tests ran 1000 tests with
		passed cleanly: 678
		passed fixed: 0
		fsck required: 0
		failed: 322
			old file persisted: 0
			file missing: 322
			file data corrupted: 0
			file metadata corrupted: 0
			other: 0

	```

17. ### generic_341 ###
	Test that if we rename a directory, create a new directory that has the old name of our former directory and is a child of the same parent directory, fsync the new inode, power fail and mount the filesystem, we see our first directory with the new name and no files under it were lost.

	**Result** : Fails on btrfs (kernel 4.4). [The moved directory and all its files are lost](https://www.spinics.net/lists/linux-btrfs/msg53591.html)

	**Output** :
	```
	Reordering tests ran 74 tests with
		passed cleanly: 30
		passed fixed: 0
		fsck required: 0
		failed: 44
			old file persisted: 0
			file missing: 44
			file data corrupted: 0
			file metadata corrupted: 0
			other: 0

	```

18. ### generic_342 ###
	We rename a file, and create a new file with the old name o the renamed file, and fsync it. If we recover after a crash now, we should see both the renamed file and the new file.

	**Result** : Fails on f2fs (kernel 4.15) and btrfs (kernel 4.4). The renamed file is lost : [f2fs](https://git.kernel.org/pub/scm/linux/kernel/git/jaegeuk/f2fs.git/commit/?id=0a007b97aad6e1700ef5c3815d14e88192cc1124), [btrfs](https://patchwork.kernel.org/patch/8694301/)

	**Output** :

	f2fs:
	```
	Timing tests ran 2 tests with
		passed cleanly: 1
		passed fixed: 0
		fsck required: 0
		failed: 1
			old file persisted: 0
			file missing: 1
			file data corrupted: 0
			file metadata corrupted: 0
			incorrect block count: 0
			other: 0

	```

	btrfs:
	```
	Reordering tests ran 1000 tests with
		passed cleanly: 974
		passed fixed: 0
		fsck required: 0
		failed: 26
			old file persisted: 0
			file missing: 26
			file data corrupted: 0
			file metadata corrupted: 0
			other: 0

	```



19. ### generic_343 ###
	If we move a directory to a new parent and later log that parent and don't explicitly log the old parent, when we replay the log we should not end up with entries for the moved directory in both the old and new parent directories.

	**Result** : Fails on btrfs (kernel 4.4). [The moved directory is present in both old and new parent](https://patchwork.kernel.org/patch/8766401/)

	**Output** :
	```
	Reordering tests ran 1000 tests with
		passed cleanly: 678
		passed fixed: 0
		fsck required: 0
		failed: 322
			old file persisted: 322
			file missing: 0
			file data corrupted: 0
			file metadata corrupted: 0
			other: 0

	```

20. ### generic_348 ###
	If we create a symlink, fsync its parent directory, power fail and mount again the filesystem, the symlink should exist and its content must match what we specified when we created it (must not be empty or point to something else)

	**Result** : Fails on btrfs (kernel 4.4). [We end up with an empty symlink](https://patchwork.kernel.org/patch/9158353/)

	**Output** :
	```
	Reordering tests ran 1000 tests with
		passed cleanly: 968
		passed fixed: 0
		fsck required: 0
		failed: 32
			old file persisted: 0
			file missing: 0
			file data corrupted: 0
			file metadata corrupted: 32
			other: 0

	```

21. ### generic_376 ###
	Rename a file without changing its parent directory, create a new file that has the old name of the file we renamed and fsync the file we renamed. Rename should work correctly and after a power failure both file should exist.

	**Result** : Fails on btrfs (kernel 4.4). [The new file created is lost](https://patchwork.kernel.org/patch/9297215/)

	**Output** :
	```
	Reordering tests ran 1000 tests with
		passed cleanly: 950
		passed fixed: 0
		fsck required: 0
		failed: 50
			old file persisted: 0
			file missing: 50
			file data corrupted: 0
			file metadata corrupted: 0
			other: 0

	```

22. ### generic_468 ###
	Test that fallocate with KEEP_SIZE followed by a fdatasync then crash, we see the right number of allocated blocks.

	**Result** : Fails on ext4 and f2fs (kernel 4.4). [The blocks allocated beyond EOF are all lost.](https://patchwork.kernel.org/patch/10120293/)

	**Output** :

	f2fs:
	```
	Timing tests ran 3 tests with
		passed cleanly: 1
		passed fixed: 0
		fsck required: 0
		failed: 2
			old file persisted: 0
			file missing: 0
			file data corrupted: 0
			file metadata corrupted: 0
			incorrect block count: 2
			other: 0

	```

	ext4:
	```
	Timing tests ran 3 tests with
		passed cleanly: 2
		passed fixed: 0
		fsck required: 0
		failed: 1
			old file persisted: 0
			file missing: 0
			file data corrupted: 0
			file metadata corrupted: 0
			incorrect block count: 1
			other: 0

	```

23. ### generic_ext4_direct_write ###
	If a buffered write extends a file, and before it is resolved, if we do a direct write, the file size should be updated correctly. In ext4 direct write path, we update i_disksize only when new eof is greater than i_size, and don't update it even when new eof is greater than i_disksize but less than i_size

	**Result** : Fails on ext4 (kernel 4.15). [The file size is 0, while block count is non zero due to direct write](https://marc.info/?l=linux-ext4&m=151669669030547&w=2)

	**Output** :
	```
	Reordering tests ran 1000 tests with
		passed cleanly: 629
		passed fixed: 0
		fsck required: 0
		failed: 371
			old file persisted: 0
			file missing: 0
			file data corrupted: 0
			file metadata corrupted: 371
			other: 0

	```

24. ### btrfs_link_unlink ###
	Suppose we remove a hard link to a file, and create a new file with the same name, followed by a fsync. If we crash now, the fsync log cannot be replayed, and makes the FS unmountable.      

	**Result** : Fails on btrfs (kernel 4.16). [Filesystem becomes unmountable.](https://www.spinics.net/lists/linux-btrfs/msg75204.html)

	**Output** :
	```
	Reordering tests ran 1000 tests with
		passed cleanly: 678
		passed fixed: 0
		fsck required: 0
		failed: 322
			old file persisted: 0
			file missing: 0
			file data corrupted: 0
			file metadata corrupted: 0
			incorrect block count: 0
			other: 0

	```


25. ### btrfs_rename_special_file ###
	Suppose we rename a special file(FIFO, character/block device or symlink), and then create a hard link with the old name of the special file. If we now persist the fsync log tree, and crash, the filesystem becomes unmountable.     

	**Result** : Fails on btrfs (kernel 4.16). [Filesystem becomes unmountable.](https://www.mail-archive.com/linux-btrfs@vger.kernel.org/msg73890.html)

	**Output** :
	```
	Reordering tests ran 1000 tests with
		passed cleanly: 633
		passed fixed: 0
		fsck required: 0
		failed: 367
			old file persisted: 0
			file missing: 0
			file data corrupted: 0
			file metadata corrupted: 0
			incorrect block count: 0
			other: 0

	```

26. ### btrfs_inode_eexist ###
	Suppose we create a file (assign a new objectid to the new inode) and persist these changes in the log tree by issuing a fsync(). If we now crash, let the log replay complete and try creating a new file, the create fails with EEXIST.     

	**Result** : Fails on btrfs (kernel 4.16). This is because the new file inode will be assigned the same objectid as the file present in fsync log, because the highest objectid in use was not recalculated after log replay. [File create fails with EEXIST.](https://www.mail-archive.com/linux-btrfs@vger.kernel.org/msg73890.html)

	**Output** :
	```
	Reordering tests ran 1000 tests with
		passed cleanly: 678
		passed fixed: 0
		fsck required: 0
		failed: 322
			old file persisted: 0
			file missing: 0
			file data corrupted: 0
			file metadata corrupted: 322
			incorrect block count: 0
			other: 0


	```
