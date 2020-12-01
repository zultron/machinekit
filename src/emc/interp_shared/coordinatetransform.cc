/**
 * @file coordinatetransform.cc
 * Methods for coordinate transform API used in interp / emccanon.
 *
 * Typically, internals of the various transforms are public because the
 * interpreter has to do incremental updates. In some cases we can't do any
 * better encapsulation-wise than what G-code / canon API allows. It's much
 * easier to just directly poke an offset than try to anticipate all the
 * possible changes with a restricted public API.
 *
 * @author Robert W. Ellenberg <rwe24g@gmail.com>
 *
 * @copyright Copyright 2019, Robert W. Ellenberg
 *
 * This source code is released for free distribution under the terms of the
 * GNU General Public License (V2) as published by the Free Software Foundation.
 */
