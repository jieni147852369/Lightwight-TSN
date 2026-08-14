#ifndef HAVE_CONFIG_H
#define HAVE_CONFIG_H

#include <getopt.h>
#include <sys/queue.h>

#include "ds.h"
#include "dm.h"
#include "filter.h"
#include "interface.h"
#include "mtab.h"
#include "transport.h"
#include "servo.h"
#include "sk.h"

struct config {
	/* configured interfaces */
	STAILQ_HEAD(interfaces_head, interface) interfaces;
	int n_interfaces;

	/* for parsing command line options */
	struct option *opts;

	/* hash of all non-legacy items */
	struct hash *htab;

	/* unicast master tables */
	STAILQ_HEAD(ucmtab_head, unicast_master_table) unicast_master_tables;
};

int config_read(const char *name, struct config *cfg);
struct interface *config_create_interface(const char *name, struct config *cfg);
void config_destroy(struct config *cfg);

/* New, hash table based methods: */

struct config *config_create(void);

double config_get_double(struct config *cfg, const char *section,
			 const char *option);

int config_get_int(struct config *cfg, const char *section,
		   const char *option);

char *config_get_string(struct config *cfg, const char *section,
			const char *option);

int config_harmonize_onestep(struct config *cfg);

static inline struct option *config_long_options(struct config *cfg)
{
	return cfg->opts;
}

int config_parse_option(struct config *cfg, const char *opt, const char *val);

int config_set_double(struct config *cfg, const char *option, double val);

int config_set_section_int(struct config *cfg, const char *section,
			   const char *option, int val);

static inline int config_set_int(struct config *cfg,
				 const char *option, int val)
{
	return config_set_section_int(cfg, NULL, option, val);
}

int config_set_string(struct config *cfg, const char *option,
		      const char *val);

#endif
