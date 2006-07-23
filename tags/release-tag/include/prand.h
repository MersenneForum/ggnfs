/**************************************************************/
/* prand.h                                                    */
/* Copyright 2003, Chris Monico.                              */
/**************************************************************/
/*  This file is part of GGNFS.
*
*   GGNFS is free software; you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation; either version 2 of the License, or
*   (at your option) any later version.
*
*   GGNFS is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with Foobar; if not, write to the Free Software
*   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifndef _PRAND_H
#define _PRAND_H


#define prandseed(_s1, _s2, _s3) { _rstate1=(_s1); _rstate2=(_s2); _rct=(_s3); }



static unsigned long _rstate1=1, _rstate2=2, _rct=0;

#define _RPADSIZE 1249
static unsigned long _RPAD[] = {
 0xe403be9e,0xb42f11af,0xa52fdb10,0xe297f65e,0xfc5bb24b,0x3d436dfb,0x7abbdefb,0x8c5869c0,
 0xbff8ce7b,0x82ca16cb,0x487fa533,0xc559aaa8,0x446eb6d0,0x98b207cb,0x1121bfd2,0x88395d72,
 0xe733a2e0,0x2bd54254,0xd62826b3,0xe486d27b,0x8354cfe0,0x2aa5db43,0x072c0379,0x639b6495,
 0xef284e2a,0x3809c491,0x7696ba22,0x9229a23b,0x838c0d1f,0x1d0b5b9a,0xe20bfca3,0x9fbd9956,
 0x692fb561,0xbebb2c44,0xd0c4c24a,0xedd1ac9b,0x9bc03de3,0xef62e1b1,0x7fd42117,0xec1ce87c,
 0x66633ab7,0x5730ed8a,0xf3731a97,0x09a8522a,0x2c9ab1c1,0x7ce95c02,0x5c77a593,0x84e5dcf8,
 0x4e6b61c7,0x8d28edc8,0x16a19750,0x13b57f89,0xdcc0413d,0x93cd68f1,0x5b008575,0xa274d91e,
 0x20d448de,0x5a744895,0x5bb0dbc0,0x2962d698,0x3e9871ed,0xc4fc86d6,0xa1e538b7,0xdd09a68a,
 0xc9b7de9b,0x6e405bd2,0xfea62531,0xf0991820,0x84ec3d6c,0x8051df3d,0x9f15aa4d,0x5198fd0c,
 0xdbffb22c,0x58b0c2aa,0x652132ea,0xd6bb570d,0x52ff5fff,0x075514c5,0x2470f5cf,0x9f3dfd8a,
 0x4c7aa8ed,0x5af88408,0x1348461d,0xe26f6e09,0xc13c4271,0xacdbe400,0x0ba3d47d,0x57c1ac23,
 0x3adfc6f6,0x3dfd5f01,0x16a03208,0xa496e90a,0x0c3d03e7,0x54706a6e,0x3f41af73,0x801bc628,
 0x5470424d,0xabce0555,0x001b5e96,0xf5bcfb4d,0x4ca673f2,0xd59a492d,0x379e127b,0x00ae7192,
 0x8cae7301,0x9ffdfac9,0xf3491d54,0xb11e68f2,0x30df249f,0x6b9b4ff0,0xc0c0b8e0,0xc849c4da,
 0x2c83dd63,0x799fa2fa,0xc993fe59,0xb69fe0b4,0x22388b4f,0xef0d7a18,0xacecd582,0xafb9d4e6,
 0xb43c2825,0x53a7a4cc,0x8e881114,0xc82dd2d1,0xd771c017,0x5498993f,0x5e84ddbc,0x7cfcb0b7,
 0x1e4971cb,0x4c32d727,0xf424ff32,0xa835c029,0x70ef7815,0xcbbab416,0x8607d42c,0x236dce11,
 0xc067e408,0x0557ba12,0x15c81100,0x7828bd5c,0xc79d3f1b,0x2d92e84a,0xa1388575,0x5cd9d80e,
 0xab9317ed,0x1edb3faa,0x4293e87c,0xee2d02c9,0xc0b69370,0xbc50da6b,0x073a0509,0x27e6f24e,
 0xbddc9487,0xab4ffe2a,0x1bb7647e,0xe1e1f802,0x4ec2ef81,0x02c6e65e,0x8c84db17,0xe0c15283,
 0x619e8ab1,0x33570c56,0x2dd762b2,0x00f3d2ef,0x0d9e3f40,0x05a2c7be,0x9bc54af8,0x2bec73f6,
 0xdbd32931,0xf24b7cc9,0xfd0c8405,0xb99c2c40,0xb3427196,0x6e564113,0x5573555b,0xb7f7e56b,
 0x3dc6316e,0x2eb8a10d,0xc9a46dc1,0x4c581fc6,0x66d2fac3,0x6a28a043,0x3c2ddd29,0x358c2cdf,
 0x8d4734ba,0xc233c31c,0x714eeec5,0xbc05c601,0xd669e012,0xc39e17f8,0x38a1c626,0xe877b3dd,
 0x505f95e2,0xa081c3b3,0x258d909b,0x082d515f,0xcc87d930,0xb6a94008,0x7dfeccf7,0x767011fb,
 0x579bc63f,0x0363408c,0x3fa13b29,0xcebe810b,0xe5a8070a,0xccad4488,0x6a3b3fd9,0x8cc5274d,
 0xfff948d0,0xd3843077,0xa60168ba,0xebbc95aa,0x615aa000,0x1d78abe9,0xa7d1c2cb,0xf632af60,
 0xc0deee7b,0x69b777cf,0x2cfd7f84,0x41157dc7,0x9eb3c3aa,0xc013d8c7,0xe8702e0a,0xba5745c9,
 0x052a9b4a,0xd0d5ed19,0xaaabb9c1,0xe291e6ac,0x58445479,0x7baee1b9,0x447d32ad,0xa218a774,
 0xbfa01e3a,0x938c171a,0x463a8a7b,0xdf0b86ea,0x1ff75297,0x8787a5db,0x9f69787e,0x2e6a0f7b,
 0x2a7c1572,0x7e7ddeb6,0xe736341d,0x6191b524,0x52305f1e,0xd41729b1,0xfa3be0bd,0x5aac6f43,
 0xafa30c26,0xe04c3beb,0xfcb6b937,0xfc0cea23,0xa49d8516,0x10301555,0x9b46f963,0xadb1cc41,
 0x391cb0f2,0xc227ce1d,0x85c5f826,0x1aeca724,0x9572cc36,0x8e523aa1,0xdc04f19f,0x0f8af6a4,
 0xca37f7eb,0x5f19d36b,0xadcf9f92,0x53bc9e62,0xc2f63411,0x906f265a,0x85c1afbf,0xdac3b163,
 0x8118cae8,0x2c25145f,0xf8ef492a,0xf67d9df4,0xbc5bb5e8,0x570e8895,0x97e4e619,0xd4e867b7,
 0xcaec1cf3,0x9647b59c,0xb963f2b4,0x4feea564,0xde1644f5,0x12f07afd,0x8226d199,0xed8a9b55,
 0xf4cb8a10,0x6665cdc4,0x0b98fcb2,0x4b4722b4,0x0c0c873e,0x407c2e4f,0x9db96167,0x3672c879,
 0xf2ec4e72,0xcbbe6e32,0x0817226c,0xdc97b16d,0x97a59337,0xf5e8c3bc,0x9c05c548,0xe95240e9,
 0x28108d71,0xd6903d0f,0x332f08a1,0xd3a42e64,0xcb7af68e,0x08675d8f,0xdc247a9e,0x9668af58,
 0x1ff4575e,0xf57ac2f9,0xb65dd766,0x545096ec,0x64e64828,0xb328892a,0xa91542eb,0xa9e76f23,
 0x10582ab8,0xbfd0de3d,0x6c38f218,0x0d85775d,0x1e8d2698,0x79b28e8f,0x90cde65e,0xb98b2057,
 0x4a4fd1cd,0xb567a3c8,0x78f84a3e,0x49b773a7,0xf031c258,0x6feda4af,0x0aaf37ff,0x5e8e433b,
 0x2bba4645,0xff2871e4,0xe354c3f6,0x2e329193,0xe183ffc9,0x6f7c4c63,0x0c3d6607,0x14a1a4af,
 0xbe37e3b7,0x21390f85,0xcb7c1e0f,0x0c03b808,0x8fcec178,0xe212edc1,0xdcd26df2,0xaa9e0849,
 0xc5ced4fb,0x6517eff9,0x26789687,0x81ab75ae,0x304e2f44,0xb7ba7f93,0x670d1e4d,0x61d9eb60,
 0x1a00099b,0x01f84a68,0x2baee4c4,0x6052a6f2,0xcdf6b639,0x8e8df45f,0xcb400368,0x914e1fba,
 0xa47c2fce,0xbb517e71,0x44b33270,0xa6b8d205,0x7ab570af,0xe4539a69,0xb64bab05,0x8a2af50e,
 0x6d08ea10,0xab3f0249,0x3790bb22,0xa6951045,0x6c58dc7c,0x165295b7,0x957c830d,0x80bae1e8,
 0x0b623139,0x3f45658b,0x20961774,0x856798f2,0xe1d134b4,0x519f4ed6,0x6c716797,0x525ddc3c,
 0xde4d3567,0xd3af3f4e,0xb9abe0e6,0x65b4a83c,0xd1327382,0xbf030e26,0xeccc6c64,0xade4227f,
 0x59697e05,0x8309a91e,0x499dc23d,0x58c9b805,0x83ad3a71,0x9c94c17f,0xaf862173,0x0055a522,
 0x79f181d4,0x7d169ac2,0x21d9843e,0xb65bdafd,0xa43fbd55,0xe56acd18,0xbf8ad149,0x18e1e2e0,
 0x00d3f991,0xaa3db668,0xdbcd12a9,0x5ff6036a,0x1395ead2,0xc0760f67,0x63d10a28,0x0a2d09f0,
 0xd04922e7,0x615f54ce,0x1aaa2e82,0x23e500c0,0x82f54892,0x98f8c348,0x71531f62,0xb21fa020,
 0x749eb381,0x2f581387,0xafb9aa5a,0x0adfe181,0xcce20a6f,0x3b08a4f3,0x2c567ef2,0x375fab9d,
 0x260357de,0xcc157412,0x987d96c7,0x881f3fda,0x83a89a5b,0x8b4bdff0,0xd256c868,0x10160cdd,
 0xbdee6201,0x2b46780d,0xb1ac7691,0x61e4bc92,0x5f090cd6,0x30863c81,0x150e8033,0x8675a9a1,
 0xddec2188,0xbb0e0fbd,0x1840cd1d,0x2d6b9747,0x521409f1,0x0f8c3b0a,0x4bc8536b,0xbcd9e785,
 0x140f2a5a,0x109480a0,0xf28dad32,0x2ad8b1a6,0xe2f290a4,0x23122e0f,0x146276d0,0x9d5bf9a2,
 0x58b86841,0xe6a63f1d,0x76425042,0xa8ac167d,0xc86bf06f,0xd3f9061a,0xd678da83,0xc1d7cf46,
 0x2e756e76,0xce3eb26f,0xa0c425f9,0xc2dbd9f1,0x7ac97656,0xf8139710,0xcd40da3f,0xfea8d269,
 0x72d97fff,0x5d12a4c3,0xe76c4b9d,0x1c973319,0xac4c9e30,0x40c37eb0,0x6a5dbada,0x2be4a7e5,
 0xf0ee9829,0xabbfb315,0xe38c216c,0xe788a800,0x7b750647,0x2df18959,0x6e88274c,0x996914b0,
 0x1af0dea7,0xfb7ac5fd,0x45f4737c,0xc96cae52,0xb03bebd1,0x12823ada,0xa9ccdc92,0x40ec6f99,
 0x4033546e,0x09f907e0,0xfb04665e,0xed3dd9cd,0x27855c3c,0xc61da709,0xf5d7c0b7,0xe99b4a4a,
 0xd745f994,0x26e8570e,0x9e53a28c,0x0d224145,0xc3592e43,0x1f8d6534,0xf70c8207,0xe163a352,
 0x035bbb1b,0x5e79c826,0x811de86b,0xf2ed167a,0x6034959d,0x58fbc6b3,0xe2ce1651,0x2fbae9eb,
 0xff728fa1,0x5d36ea31,0x5ff5cf95,0xc77c7a31,0x7775b4eb,0xf1e1577e,0x9e4dc61e,0xe2e189d2,
 0x05258030,0x0526c2ec,0xd3a4a2b4,0x7534fb7b,0x95d1d542,0x23f8be18,0xc0849461,0xbbb558d0,
 0xe940d66a,0xe246ca31,0x8cf9a7c5,0xfa1d7344,0x1bc6baab,0x92d49e96,0xf747d1ab,0x9b0dedef,
 0xc5b66ab6,0xada62eec,0x77fdcc12,0x22a456bd,0x9af7f6d0,0xcae60941,0xcdf03626,0x1ef630a6,
 0xb106e381,0x62525ee2,0x1c73917a,0x3e5fa544,0x76358e0d,0x720af99b,0xdf0c8a9b,0x6bb69ffc,
 0x3799f2af,0x3cb774d3,0x66c4713a,0x69791983,0xc69c0617,0x556f2b24,0xd15f499e,0xa0dd02ff,
 0x705a77a1,0x86922954,0xe2f90542,0xd09ecb30,0x92b96544,0x0c46cfcc,0x5c5c1f45,0xac87a9a3,
 0x35b9d0e9,0x50399723,0xe960dfde,0xffc1739f,0x0eaf2ae3,0x25f5c3c4,0x7f783272,0xa87bef65,
 0xb5e419b7,0x5a867948,0x0324368d,0x755290ff,0x68db4566,0xbd5b8cf7,0x6bcac4fc,0xf92dab61,
 0x3aca0c4a,0xe400e42d,0xb775d0ba,0x573ba401,0xdbe5b73f,0xfbe44240,0x5936d3c8,0x5def7ea0,
 0xd9885249,0x4fae956e,0xcb4b3aa5,0x43f422aa,0x3da27abc,0x31a40d27,0x1d264409,0xd0de108e,
 0x96429a72,0x40930453,0xd91a9d8b,0xde5f2216,0xdb45b117,0xf319bc71,0x6c41ed07,0x0c8e8be7,
 0xd4cc9668,0xe326bf46,0xa267e150,0x694be2ab,0x2e0431c8,0x748b97c1,0x60a2be19,0x72c2bce5,
 0xb02ce046,0x2e0804a3,0x49490f2c,0x0d802133,0x676d4543,0x9ddd1de1,0x14f1d007,0xe17e9ced,
 0x175c40f6,0xc3f12b7a,0xc82a49c9,0xd644149d,0x5d34c224,0x59351a8b,0x7c945839,0x09b527bf,
 0x1aa5ec7d,0x3e65e512,0x0f32a543,0x7f49d48a,0x9e72c735,0x95f801c5,0x324a17c4,0xc8f1736e,
 0xa85a6b87,0x6867ea61,0xed0a7d4e,0x3ce19650,0xa45f686c,0x66d84607,0x5d3d06d7,0x8128d9a5,
 0xdb7995f8,0x0d43958f,0x6351aba8,0x0bfe89bc,0xe5982220,0xa307b652,0x001e73a9,0x3a47f642,
 0x4e910fef,0x48354753,0xcca78ede,0x9ac144a5,0xa11ab43f,0xd8a0886b,0x18492136,0xa79ee24c,
 0xa916cc34,0x36266639,0x5d8d69bd,0x99cc2403,0x8d382a6c,0x17db5a50,0xe9f7add3,0xc188320b,
 0x90daf98e,0xaed7a66f,0x553a6f08,0x3c3994da,0x89eea802,0x459f2b84,0x09218b68,0x3dc0adb1,
 0x063dd3e2,0xdd358528,0xbfb65971,0xf54222ad,0x9f6cb5ab,0xd7d7d840,0xe3745737,0xb6d22de8,
 0x5dcd7cfe,0xb99d05b6,0xf2b71440,0x718a2190,0x6bfb5743,0xd6a2d4ef,0x48a3948f,0xad10dbfa,
 0xa5de6264,0xa92297d0,0xde0e4c44,0xc1a04804,0x093c9005,0x041d8351,0xaf75e7e5,0x4fa91158,
 0xb1541063,0x9b46256e,0x8e962e93,0x9f399f1f,0x80c3f7d5,0x4eadf568,0xdbcc13a1,0x06e7c097,
 0x6892ad3e,0xc54cff55,0x83d7cd00,0xabd9e6a7,0x395698d1,0x63851e67,0xd9c8aab6,0x8daeb6b7,
 0xaea6e71d,0xa28af415,0x022d8990,0x4bbe74a3,0x02f10fe7,0xb06efd6f,0xb440dab9,0xf43d569c,
 0x27324dfd,0x73bf6648,0xbf2bde7e,0x1d91058b,0xf6042789,0x738792ea,0x07080544,0x2223e407,
 0x979f3f78,0xd835be88,0x7fc7a122,0xaeec8261,0x76b85c81,0x4b2028c9,0x24c4319f,0x21f167ce,
 0x4797ce16,0x3094b96b,0xd2e0d0ad,0xeb59faa0,0x898ec4bf,0xd030eb30,0xafefbdba,0x6ad1ad95,
 0xf0272fb0,0x00edfea7,0xc8f01bfc,0x723f2c22,0x78f93557,0xc4288e98,0xa8545fe8,0xb55b8402,
 0x31c4e36b,0x387e7c56,0xac1c4187,0xcfb4b61a,0xd63424f7,0x44691791,0xb662892c,0x4f751f44,
 0xf8b1c419,0x599189da,0xa76d8066,0xb7f96758,0x66eb4cd1,0xd891d6c8,0x6613955e,0x0804fa73,
 0x6b98eadb,0x61795791,0x18bb995f,0xef5e97c5,0x06262048,0x14508830,0xf033e5e1,0xeffe351a,
 0xd7cf043f,0x7d821c2c,0x9499ed9a,0x1e02019a,0x84354b37,0x0333552c,0x9834f311,0xf8743dbb,
 0x0cf2716b,0x14900bfc,0xb79ca3dd,0x4dd5063d,0xa8c890b6,0x228935af,0xc4e4d191,0xa2390120,
 0x51381bc9,0x3952523b,0x62d7cd1e,0x3d8fa26d,0xddc6a232,0x73fd2f51,0x63d80546,0xcf8eb6c6,
 0x9e7487d6,0x18b4b29a,0xdc6481d8,0xd4404f39,0x7f04d1f3,0x547adcd9,0x8d6cbe94,0xd6bc8eb0,
 0xa3752a1c,0x9472facf,0x52d3780b,0x76f9daad,0x403ca980,0xffd12939,0xe45ee354,0x444fc13d,
 0xbc7effd2,0x870f606a,0xade4c235,0x9c9db20c,0xea51b593,0xa703d6b7,0x726e4ff9,0x58bcf996,
 0xcc63a81a,0xd58af5ca,0xf41e14fa,0x1bbaa9d0,0x3100c2b7,0x43e0812b,0x04b3d254,0xd8655555,
 0x1d00b863,0x91e84cbc,0x1f9a5dfa,0x337ae82c,0x07d8e193,0x6f38f19e,0xf850b37a,0x5c69c1a4,
 0x929e132f,0x842e7dc1,0x21c9de5f,0x1595c730,0x7db8c368,0xbd5e8bd2,0x0cb10c49,0x9062e7ba,
 0x5b800102,0x310d8fb5,0x0dee2d61,0x59b87129,0x977ffdf2,0x2a23380e,0x706e207d,0x7a571edb,
 0x63927860,0x44fd23ff,0xac2190b8,0x16824ef2,0x7344726c,0x09f43385,0xd235d66c,0xc9bd5a1c,
 0xee20e6ef,0xbba32c9c,0x814af68d,0x0deb4404,0x21f79f9c,0xf5f572b4,0xb984988b,0x12ccf747,
 0x207dd212,0xabef12be,0x397da952,0xf0e4859e,0x31b60c41,0xcbb73dba,0xab061138,0x201dd631,
 0xf26227ef,0xf9765123,0xa15d9809,0x0f3fb744,0x70e865b1,0xacd47baa,0xbe0b1ff5,0x398b28f1,
 0x11b5ed86,0xeccc4833,0x65095491,0xd110604f,0xd45a8671,0xa64f4bb3,0x5699592d,0x40c935fd,
 0xc949997c,0xf923ff17,0xd0a98832,0x5a90e303,0x4525d223,0xb47c7d02,0xfbf676c2,0x8ccf7d49,
 0x5866909a,0xb86b12c3,0xf64046c3,0x3582e10b,0xbca41621,0x780576e6,0xf22591ab,0xad3b6dbd,
 0x8397dae8,0x59f00373,0xf8109fc7,0x62d094c1,0xd78b0bdf,0xaca6bade,0x20b00c91,0xffa61630,
 0x4eb464ea,0x5a8a243d,0xfcf90cd6,0x3205a13b,0xa280e859,0x5bab7437,0x87c165b3,0xb69acece,
 0x7791af83,0xfd41ca43,0xe9ac7b8d,0xe5aa843c,0x9679e003,0xb5a1aa5b,0xabb0e297,0xb89843ad,
 0x003be040,0xc9769065,0x0d3674b7,0xff0e76ce,0x9aef9cd7,0x878c0e66,0xcf598bae,0xdddb6118,
 0x3b52e4e5,0x76cecfc2,0x228aa4fe,0x594fad68,0xe52a1019,0x242c6dc6,0x2135b9f3,0xe7bc126b,
 0x280241d5,0x344ad531,0xd0344881,0x6635965f,0xdc1c8911,0xae0ec4f9,0x64d8e2d4,0xbeedfad4,
 0x3e21eb25,0x69b5a235,0x86ed0536,0xc24e1d4e,0x19532cb8,0xfc3624cd,0x15b6f99f,0xa4a0d886,
 0xd8bbab3b,0xdb0ef669,0x0a3396a9,0x37150697,0x3f6ea033,0xadfbfc95,0x85140195,0x9603d62e,
 0xb6d03270,0xbe1c464b,0x5d87f850,0x33e794a3,0x54abe83b,0xa62eb5da,0xd43a82ec,0x12ea50c5,
 0x4bdf4745,0x283a864e,0x9a28e77b,0xbb038ddc,0xd5a4ecf0,0xe0a7bd55,0xb43ce08a,0x65318ab6,
 0xfee22f97,0x2b482732,0x845983db,0x58d4a67a,0x4a21ab8c,0x91279d02,0xbd5d5847,0x71866bf9,
 0x60b2949e,0xe64752a4,0x57b533e8,0x795a0355,0x4f0a90c8,0x43480f68,0x3e3cac5d,0x76f391b1,
 0xd44bd4ce,0x18b7284a,0xbb8714f3,0xdb10a1f4,0xb74ac94e,0x33e871fb,0xfb4c5276,0xa61592cd,
 0xbfe78687,0x5cae1661,0x219ff219,0xd10aa4bc,0x5a0ff0fc,0xa09033f5,0x00503960,0x82f24936,
 0x2df62dbd,0x14142448,0xaa32747b,0x47c86a49,0x57fc7637,0xe56132ec,0xed267165,0xd5bcee50,
 0x8526eaa2,0x4cd09f7b,0x70c5ca43,0xb1dd63d9,0xffd59a80,0xbe813a10,0x233acd14,0x42511e56,
 0xfbaef52c,0x1d734df4,0x191b4126,0x25b302f4,0xfd4a6a13,0xddcf6541,0x79529128,0x76595fe3,
 0x20245c20,0xe370340f,0x17d87bbc,0x29497e11,0x39c60a0f,0x2c1f9147,0xf571a98c,0x20f91a98,
 0xa21cd03f,0x1aa43291,0x7e3c0e74,0x0046d6b4,0x56ab0d7d,0xc6910eee,0x51aadf8b,0x4f60bb13,
 0x911605b9,0x817d26c3,0xbddb57b7,0xb9a8e639,0xc347b301,0x6cd08bb9,0xe4d37c01,0x4127297b,
 0x21e4936f,0x14e8f81f,0x2bec9eb3,0x6f848275,0x32426de7,0x615a6d96,0xad4a9f64,0x51ab895b,
 0x23a0b63d,0xe9b0fbb1,0x2e5222b6,0xdb452ab5,0xc0f59785,0x30827a84,0x3608f135,0x3bd1803d,
 0x5cf72911,0xe3c0d69b,0xe77fe2d1,0xef79f784,0xf59ff3b5,0xf6b26e2b,0x7357368e,0x74d86efe,
 0xb5b10422,0x4cae051c,0x71ef9bfe,0x2b7c7ccd,0x0931cd6c,0x7f01c8d2,0xa11bb144,0x717729fb,
 0x7f4259ab,0x6e87d1e9,0x3d74f756,0x0f47f29e,0x45b66307,0x046f5b33,0x23b85438,0x836ec7f4,
 0xb4115d34,0x08c5fae7,0xb2684738,0x795ae420,0x21a9b4b1,0x0d7eacb8,0x27afa942,0x018c69e8,
 0x5dea7840,0x1d253314,0x724dca80,0xb5a0763c,0x10b13f6a,0xf2aa8ab5,0xda8ad32f,0x31403b92,
 0x8dc0f46e,0x59114634,0xb31b24c8,0x85a7b36a,0x38b289c8,0xf4d65444,0xb85537fb,0x3a0a624d,
 0x2b8de293,0x5c1af673,0x29c3a27c,0xfea181ef,0x4e20ab37,0x58e57667,0x2eeba689,0xe50c0801,
 0xa6f6bf52,0xee4c4f90,0xf4cab7e5,0xd6e2b315,0x0215e461,0xe73112a7,0xefcae6e6,0x343c61ec,
 0x43ed8fc3,0x3c9ba211,0xf6dbf4a9,0xcb99765d,0xe8971f3a,0x3446011a,0x431513ea,0x9b8d0725,
 0x0fb6b66f,0x90a3f76c,0x4096f6f5,0x4f51a66b,0x22d6b8c3,0xe1390847,0xaeabd341,0x87a9fe40,
 0x0fe86d61,0x39563076,0x8cf4478b,0xa2e7cda4,0xc6431bd3,0xbe326830,0x83727e43,0x90a41ee9,
 0x8bd95c8f,0x539b0fe6,0xda6632a4,0x3ec08141,0x4cdaf5bf,0x1bb14c2b,0xd8415a4a,0x394f730a,
 0x521ac13b,0x6cd58e27,0x27a2ce30,0x8dfc2010,0x6cad474f,0x09b8c0df,0x92bfb798,0x37c4d072,
 0xd73689c7,0x0c58c59e,0x8477cc8a,0x373df55b,0x345fb1c3,0x5382f2b7,0x29668557,0x1fb32059,
 0x461d2b49,0x27b11a39,0x04d54a6e,0x74c40bb0,0x1bd45c91,0x37e0513e,0xde64e3d7,0xe6342a45,
 0xf62350fa,0xe2baae4b,0x70d8fb3e,0x0989c4a3,0x116198ed,0xa88c42ac,0x1de49996,0xa65e48ce,
 0x9ccfe881};


 
  /*********************************************************/
  /* This is George Marsaglia's Multiply-With-Carry (MWC)  */
  /* PRNG that concatenates two 16-bit MWC generators:     */
  /*     x(n)=36969 * x(n-1) + carry mod 2^16              */
  /*     y(n)=18000 * y(n-1) + carry mod 2^16              */
  /* to produce a combined PRNG with a period of about     */
  /* 2^60 that seems to pass all tests of randomness. It   */
  /* uses 2 32 bits seeds  x and y which are updated on    */
  /* each call.                                            */
  /*********************************************************/
#define MRAND(_x,_y) ((((_x) = 36969 * ((_x) & 65535) + ((_x) >> 16)) << 16) + \
                            ((_y) = 18000 * ((_y) & 65535) + ((_y) >> 16)) )

/* This is the MWC, concatenated with a random pad of length 1249, */
/* to (slightly) increase the period.                              */
#define prand() (MRAND(_rstate1, _rstate2) ^ _RPAD[_rct++ % _RPADSIZE])
#define prand_01() ((double)prand()/(double)0xFFFFFFFF)


#endif

