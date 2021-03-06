/* -*- mode: C++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
// vim: ft=cpp:expandtab:ts=8:sw=4:softtabstop=4:
#ident "$Id$"
/*
COPYING CONDITIONS NOTICE:

  This program is free software; you can redistribute it and/or modify
  it under the terms of version 2 of the GNU General Public License as
  published by the Free Software Foundation, and provided that the
  following conditions are met:

      * Redistributions of source code must retain this COPYING
        CONDITIONS NOTICE, the COPYRIGHT NOTICE (below), the
        DISCLAIMER (below), the UNIVERSITY PATENT NOTICE (below), the
        PATENT MARKING NOTICE (below), and the PATENT RIGHTS
        GRANT (below).

      * Redistributions in binary form must reproduce this COPYING
        CONDITIONS NOTICE, the COPYRIGHT NOTICE (below), the
        DISCLAIMER (below), the UNIVERSITY PATENT NOTICE (below), the
        PATENT MARKING NOTICE (below), and the PATENT RIGHTS
        GRANT (below) in the documentation and/or other materials
        provided with the distribution.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
  02110-1301, USA.

COPYRIGHT NOTICE:

  TokuFT, Tokutek Fractal Tree Indexing Library.
  Copyright (C) 2007-2013 Tokutek, Inc.

DISCLAIMER:

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

UNIVERSITY PATENT NOTICE:

  The technology is licensed by the Massachusetts Institute of
  Technology, Rutgers State University of New Jersey, and the Research
  Foundation of State University of New York at Stony Brook under
  United States of America Serial No. 11/760379 and to the patents
  and/or patent applications resulting from it.

PATENT MARKING NOTICE:

  This software is covered by US Patent No. 8,185,551.
  This software is covered by US Patent No. 8,489,638.

PATENT RIGHTS GRANT:

  "THIS IMPLEMENTATION" means the copyrightable works distributed by
  Tokutek as part of the Fractal Tree project.

  "PATENT CLAIMS" means the claims of patents that are owned or
  licensable by Tokutek, both currently or in the future; and that in
  the absence of this license would be infringed by THIS
  IMPLEMENTATION or by using or running THIS IMPLEMENTATION.

  "PATENT CHALLENGE" shall mean a challenge to the validity,
  patentability, enforceability and/or non-infringement of any of the
  PATENT CLAIMS or otherwise opposing any of the PATENT CLAIMS.

  Tokutek hereby grants to you, for the term and geographical scope of
  the PATENT CLAIMS, a non-exclusive, no-charge, royalty-free,
  irrevocable (except as stated in this section) patent license to
  make, have made, use, offer to sell, sell, import, transfer, and
  otherwise run, modify, and propagate the contents of THIS
  IMPLEMENTATION, where such license applies only to the PATENT
  CLAIMS.  This grant does not include claims that would be infringed
  only as a consequence of further modifications of THIS
  IMPLEMENTATION.  If you or your agent or licensee institute or order
  or agree to the institution of patent litigation against any entity
  (including a cross-claim or counterclaim in a lawsuit) alleging that
  THIS IMPLEMENTATION constitutes direct or contributory patent
  infringement, or inducement of patent infringement, then any rights
  granted to you under this License shall terminate as of the date
  such litigation is filed.  If you or your agent or exclusive
  licensee institute or order or agree to the institution of a PATENT
  CHALLENGE, then Tokutek may terminate any rights granted to you
  under this License.
*/

#ident "Copyright (c) 2007-2013 Tokutek Inc.  All rights reserved."
#ident "The technology is licensed by the Massachusetts Institute of Technology, Rutgers State University of New Jersey, and the Research Foundation of State University of New York at Stony Brook under United States of America Serial No. 11/760379 and to the patents and/or patent applications resulting from it."

#include "concurrent_tree_unit_test.h"

namespace toku {

void concurrent_tree_unit_test::test_lkr_acquire_release(void) {
    comparator cmp;
    cmp.create(compare_dbts, nullptr);

    // we'll test a tree that has values 0..20
    const uint64_t min = 0;
    const uint64_t max = 20;

    // acquire/release should work regardless of how the
    // data was inserted into the tree, so we test it
    // on a tree whose elements were populated starting
    // at each value 0..20 (so we get different rotation
    // behavior for each starting value in the tree).
    for (uint64_t start = min; start <= max; start++) {
        concurrent_tree tree;
        tree.create(&cmp);
        populate_tree(&tree, start, min, max);
        invariant(!tree.is_empty());

        for (uint64_t i = 0; i <= max; i++) {
            concurrent_tree::locked_keyrange lkr;
            lkr.prepare(&tree);
            invariant(lkr.m_tree == &tree);
            invariant(lkr.m_subtree->is_root());

            keyrange range;
            range.create(get_dbt(i), get_dbt(i));
            lkr.acquire(range);
            // the tree is not empty so the subtree root should not be empty
            invariant(!lkr.m_subtree->is_empty());

            // if the subtree root does not overlap then one of its children
            // must exist and have an overlapping range.
            if (!lkr.m_subtree->m_range.overlaps(cmp, range)) {
                treenode *left = lkr.m_subtree->m_left_child.ptr;
                treenode *right = lkr.m_subtree->m_right_child.ptr;
                if (left != nullptr) {
                    // left exists, so if it does not overlap then the right must
                    if (!left->m_range.overlaps(cmp, range)) {
                        invariant_notnull(right);
                        invariant(right->m_range.overlaps(cmp, range));
                    }
                } else {
                    // no left child, so the right must exist and be overlapping
                    invariant_notnull(right);
                    invariant(right->m_range.overlaps(cmp, range));
                }
            }

            lkr.release();
        }

        // remove everything one by one and then destroy
        keyrange range;
        concurrent_tree::locked_keyrange lkr;
        lkr.prepare(&tree);
        invariant(lkr.m_subtree->is_root());
        range.create(get_dbt(min), get_dbt(max));
        lkr.acquire(range);
        invariant(lkr.m_subtree->is_root());
        for (uint64_t i = 0; i <= max; i++) {
            range.create(get_dbt(i), get_dbt(i));
            lkr.remove(range);
        }
        lkr.release();
        tree.destroy();
    }

    cmp.destroy();
}

} /* namespace toku */

int main(void) {
    toku::concurrent_tree_unit_test test;
    test.test_lkr_acquire_release();
    return 0;
}
