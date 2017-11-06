..
.. NB:  This file is machine generated, DO NOT EDIT!
..
.. Edit vmod.vcc and run make instead
..

.. role:: ref(emphasis)

.. _vmod_esiextra(3):

=============
vmod_esiextra
=============

-----------------------
Varnish Esiextra Module
-----------------------

:Manual section: 3

SYNOPSIS
========

import esiextra [from "path"] ;


DESCRIPTION
===========

This vmod contains tools to improve varnish ESI processing
capabilities like:

* generate ``ETag`` backend response headers
* track the most recent ``Last-Modified`` date of all ESI includes.

It requires chunked encoding trailer support, which, as of 2017-11-06,
has not been merged to varnish-cache. See
https://github.com/varnishcache/varnish-cache/pull/2477

Example
    ::

	import esiextra;
	# TODO

See file ``vtc/esi_recursive_full.vtc`` for a full blown usage example.

CONTENTS
========

* VOID bodyhash(HEADER)
* lm()

.. _func_bodyhash:

bodyhash
--------

::

	VOID bodyhash(HEADER)

Hash the received body and write a hex-encoded string into HEADER.

This function may only be called in ``vcl_backend_response{}`` and can
only change ``beresp.http.*`` headers.

If Trailer support is enabled, by setting ``beresp.http.Trailer`` to
contain the name of HEADER, it will be used.

Otherwise a placeholder header will be added to the cache object and
overwritten afterwards. For this to work, it needs to disable
streaming mode.

For now, streaming mode is also disabled when Trailer support is
enabled. This restriction may be lifted in the Future.

.. _obj_lm:

lm
--

::

	new OBJ = lm()

Create an object to track the most reacent ``Last-Modified`` time during
the entirity of an ESI request including all sub-requests.

.. _func_lm.inspect:

lm.inspect
----------

::

	BOOL lm.inspect(TIME)

Inspect time vs. saved time. If new time is more recent than saved
time, update and return true. Otherwise return false.

.. _func_lm.update:

lm.update
---------

::

	VOID lm.update(TIME)

same as inspect without a return value.

.. _func_lm.get:

lm.get
------

::

	TIME lm.get()

get the saved time.

SEE ALSO
========

``vcl``\(7),
``varnishd``\(1),
``varnishesiextra``\(1)

COPYRIGHT
=========

::

  Copyright 2017 UPLEX Nils Goroll Systemoptimierung
  All rights reserved
 
  Author: Nils Goroll <nils.goroll@uplex.de>
 
  License: See the LICENSE file which should have come with this distribution
 

