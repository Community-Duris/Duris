#ifndef MATERIAL_RARITY_H
#define MATERIAL_RARITY_H

/*
 * Write a read-only, static object-template composition report.  The report
 * counts one contribution for every distinct material ingredient needed by a
 * qualifying template; declared recipe quantities remain available in the
 * per-template CSV for later supply/demand modelling.
 */
void write_material_rarity_report(const char *output_dir);

#endif
