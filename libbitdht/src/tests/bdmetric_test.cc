/*
 * bitdht/bdmetric_test.cc
 *
 * BitDHT: An Flexible DHT library.
 *
 * Copyright 2010 by Robert Fernie
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License Version 3 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA.
 *
 * Please report all bugs and problems to "bitdht@lunamutt.com".
 *
 */


#include "bitdht/bdpeer.h"
#include "bitdht/bdstddht.h"
#include <iostream>
#include <stdio.h>

int main(int argc, char **argv)
{

	/* create some ids */
	bdId id1;
	bdId id2;
	bdId id3;
	bdId id4;
	bdId id5;
	bdId id6;

	bdStdRandomId(&id1);
	bdStdRandomId(&id2);
	bdStdRandomId(&id3);
	bdStdRandomId(&id4);
	bdStdRandomId(&id5);
	bdStdRandomId(&id6);

	fprintf(stderr, "id1:");
	bdStdPrintId(std::cerr,&id1);
	fprintf(stderr, "\n");

	fprintf(stderr, "id2:");
	bdStdPrintId(std::cerr,&id2);
	fprintf(stderr, "\n");

	fprintf(stderr, "id3:");
	bdStdPrintId(std::cerr,&id3);
	fprintf(stderr, "\n");

	fprintf(stderr, "id4:");
	bdStdPrintId(std::cerr,&id4);
	fprintf(stderr, "\n");

	fprintf(stderr, "id5:");
	bdStdPrintId(std::cerr,&id5);
	fprintf(stderr, "\n");

	fprintf(stderr, "id6:");
	bdStdPrintId(std::cerr,&id6);
	fprintf(stderr, "\n");

	/* now do the sums */
	bdMetric met;
	bdMetric met2;
	int bdist = 0;
	bdStdDistance(&(id1.id), &(id2.id), &met);

	fprintf(stderr, "1^2:");
	bdStdPrintNodeId(std::cerr,&met);
	fprintf(stderr, "\n");
	bdist = bdStdBucketDistance(&met);
	fprintf(stderr, " bucket: %d\n", bdist);

	bdStdDistance(&(id1.id), &(id3.id), &met2);
	bdist = bdStdBucketDistance(&met2);

	fprintf(stderr, "1^3:");
	bdStdPrintNodeId(std::cerr,&met2);
	fprintf(stderr, "\n");
	fprintf(stderr, " bucket: %d\n", bdist);

	int c1 = met < met2;
	int c2 = met2 < met;

	fprintf(stderr, "1^2<1^3? : %d  1^3<1^2?: %d\n", c1, c2);


	bdStdDistance(&(id1.id), &(id4.id), &met2);
	bdist = bdStdBucketDistance(&met2);

	fprintf(stderr, "1^4:");
	bdStdPrintNodeId(std::cerr,&met2);
	fprintf(stderr, "\n");
	fprintf(stderr, " bucket: %d\n", bdist);

	c1 = met < met2;
	c2 = met2 < met;

	fprintf(stderr, "1^2<1^4? : %d  1^4<1^2?: %d\n", c1, c2);

	bdStdDistance(&(id1.id), &(id5.id), &met);
	bdist = bdStdBucketDistance(&met);

	fprintf(stderr, "1^5:");
	bdStdPrintNodeId(std::cerr,&met);
	fprintf(stderr, "\n");
	fprintf(stderr, " bucket: %d\n", bdist);

	bdStdDistance(&(id1.id), &(id6.id), &met);
	bdist = bdStdBucketDistance(&met);

	fprintf(stderr, "1^6:");
	bdStdPrintNodeId(std::cerr,&met);
	fprintf(stderr, "\n");
	fprintf(stderr, " bucket: %d\n", bdist);

	bdStdDistance(&(id2.id), &(id3.id), &met);
	bdist = bdStdBucketDistance(&met);

	fprintf(stderr, "2^3:");
	bdStdPrintNodeId(std::cerr,&met);
	fprintf(stderr, "\n");
	fprintf(stderr, " bucket: %d\n", bdist);


	fprintf(stderr, "id1:");
	bdStdPrintId(std::cerr,&id1);
	fprintf(stderr, "\n");

	fprintf(stderr, "id2:");
	bdStdPrintId(std::cerr,&id2);
	fprintf(stderr, "\n");

	fprintf(stderr, "id3:");
	bdStdPrintId(std::cerr,&id3);
	fprintf(stderr, "\n");

	fprintf(stderr, "id4:");
	bdStdPrintId(std::cerr,&id4);
	fprintf(stderr, "\n");

	fprintf(stderr, "id5:");
	bdStdPrintId(std::cerr,&id5);
	fprintf(stderr, "\n");

	fprintf(stderr, "id6:");
	bdStdPrintId(std::cerr,&id6);
	fprintf(stderr, "\n");


	return 1;
}


