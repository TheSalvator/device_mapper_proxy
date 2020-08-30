#include <linux/module.h>
#include <linux/init.h>
#include <linux/bio.h>
#include <linux/device-mapper.h>
#include <linux/kobject.h>
#include <linux/blk_types.h>

#define DM_MSG_PREFIX "device mapper proxy"

static int dmp_ctr( struct dm_target *ti,unsigned int argc,char **argv );
static void dmp_dtr(struct dm_target *ti);
static int dmp_map(struct dm_target *ti, struct bio *bio);
static ssize_t volumes_show( struct kobject *kobj, struct kobj_attribute *attr, char *buf );
static ssize_t volumes_store( struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count );
static int __init dmp_init(void);
static void __exit dmp_exit(void);

static struct target_type dmp_target = {
    .name = "dmp",
    .version = {1, 0, 0},
    .module = THIS_MODULE,
    .ctr = dmp_ctr,
    .dtr = dmp_dtr,
    .map = dmp_map,
};

static struct kobject *stat;

static struct kobj_attribute volumes_attribute = __ATTR( volumes, 0444, volumes_show, volumes_store );

static struct attribute *attrs[] = {
    &volumes_attribute.attr,
    NULL
};

static struct attribute_group attr_group = {
    .attrs = attrs
};

struct dmp_c {
    struct dm_dev *dev;
};

unsigned long long reads = 0;
unsigned long long reads_sum = 0;
unsigned long long reads_avg_sz = 0;
unsigned long long writes = 0;
unsigned long long writes_sum = 0;
unsigned long long writes_avg_sz = 0;

static int dmp_ctr( struct dm_target *ti,unsigned int argc,char **argv ) {
    struct dmp_c *dmp;

    if ( argc != 1 ) {
        ti->error = "Invalid argument count";
        return -EINVAL;
    }

    dmp = kmalloc( sizeof( struct dmp_c ), GFP_KERNEL );
    if ( dmp == NULL ) {
        ti->error = "Cannot allocate linear context";
        return -ENOMEM;
    }

    if (dm_get_device(ti, argv[0], dm_table_get_mode(ti->table), &dmp->dev)) {
        ti->error = "Device lookup failed";
        return -EINVAL;
    }

    ti->private = dmp;

    return 0;
}

static void dmp_dtr(struct dm_target *ti) {
    struct dmp_c *dmp = ( struct dmp_c* )ti->private;
    dm_put_device(ti, dmp->dev);
    kfree( dmp );
}

static int dmp_map(struct dm_target *ti, struct bio *bio) {
    struct dmp_c *dmp = ( struct dmp_c* )ti->private;
    unsigned int tmp_blk_sz = bio->bi_iter.bi_size;

    switch ( bio_op( bio ) ) {
        case REQ_OP_READ:
            reads_sum += tmp_blk_sz;
            reads++;
            reads_avg_sz = reads_sum / reads;
            break;
        case REQ_OP_WRITE:
            writes_sum += tmp_blk_sz;
            writes++;
            writes_avg_sz = writes_sum / writes;
            break;
        default:
            break;
    }

    bio_set_dev( bio, dmp->dev->bdev );
    submit_bio( bio );

    return DM_MAPIO_SUBMITTED;
}

static ssize_t volumes_show( struct kobject *kobj, struct kobj_attribute *attr, char *buf ) {
    ssize_t count = 0;

    count += sprintf( buf, "read:\n");
    count += sprintf( buf + count, "\treqs: %llu\n", reads );
    count += sprintf( buf + count, "\tavg size: %llu\n", reads_avg_sz );

    count += sprintf( buf + count, "write:\n");
    count += sprintf( buf + count, "\treqs: %llu\n", writes );
    count += sprintf( buf + count, "\tavg size: %llu\n", writes_avg_sz );

    count += sprintf( buf + count, "total:\n");
    count += sprintf( buf + count, "\treqs: %llu\n", reads + writes );
    count += sprintf( buf + count, "\tavg size: %llu\n", ( reads_avg_sz + writes_avg_sz )/2 );

    return count;
}

static ssize_t volumes_store( struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count ) {
    return 0;
}

static int __init dmp_init(void) {
    int r = dm_register_target( &dmp_target );
    if ( r < 0 ) {
        DMERR( "Register failed %d", r );
    }

    stat = kobject_create_and_add( "stat", &THIS_MODULE->mkobj.kobj );
    if ( !stat ) {
        return -ENOMEM;
    }

    r = sysfs_create_group( stat, &attr_group );
    if ( r ) {
        kobject_put( stat );
    }

    return r;
}

static void __exit dmp_exit(void) {
    dm_unregister_target( &dmp_target );
    kobject_put( stat );
}

module_init(dmp_init)
module_exit(dmp_exit)

MODULE_AUTHOR("Ilya Kazakov <kazakovilya97@gmail.com>");
MODULE_DESCRIPTION(DM_NAME " device mapper proxy");
MODULE_LICENSE("GPL");
