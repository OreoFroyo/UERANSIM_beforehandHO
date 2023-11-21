/*
 * Generated by asn1c-0.9.29 (http://lionet.info/asn1c)
 * From ASN.1 module "NR-RRC-Definitions"
 * 	found in "asn/nr-rrc-15.6.0.asn1"
 * 	`asn1c -fcompound-names -pdu=all -findirect-choice -fno-include-deps -gen-PER -no-gen-OER -no-gen-example -D rrc`
 */

#include "ASN_RRC_NZP-CSI-RS-Resource.h"

#include "ASN_RRC_CSI-ResourcePeriodicityAndOffset.h"
/*
 * This type is implemented using NativeEnumerated,
 * so here we adjust the DEF accordingly.
 */
static int
memb_ASN_RRC_powerControlOffset_constraint_1(const asn_TYPE_descriptor_t *td, const void *sptr,
			asn_app_constraint_failed_f *ctfailcb, void *app_key) {
	long value;
	
	if(!sptr) {
		ASN__CTFAIL(app_key, td, sptr,
			"%s: value not given (%s:%d)",
			td->name, __FILE__, __LINE__);
		return -1;
	}
	
	value = *(const long *)sptr;
	
	if((value >= -8 && value <= 15)) {
		/* Constraint check succeeded */
		return 0;
	} else {
		ASN__CTFAIL(app_key, td, sptr,
			"%s: constraint failed (%s:%d)",
			td->name, __FILE__, __LINE__);
		return -1;
	}
}

static asn_per_constraints_t asn_PER_type_ASN_RRC_powerControlOffsetSS_constr_5 CC_NOTUSED = {
	{ APC_CONSTRAINED,	 2,  2,  0,  3 }	/* (0..3) */,
	{ APC_UNCONSTRAINED,	-1, -1,  0,  0 },
	0, 0	/* No PER value map */
};
static asn_per_constraints_t asn_PER_memb_ASN_RRC_powerControlOffset_constr_4 CC_NOTUSED = {
	{ APC_CONSTRAINED,	 5,  5, -8,  15 }	/* (-8..15) */,
	{ APC_UNCONSTRAINED,	-1, -1,  0,  0 },
	0, 0	/* No PER value map */
};
static const asn_INTEGER_enum_map_t asn_MAP_ASN_RRC_powerControlOffsetSS_value2enum_5[] = {
	{ 0,	4,	"db-3" },
	{ 1,	3,	"db0" },
	{ 2,	3,	"db3" },
	{ 3,	3,	"db6" }
};
static const unsigned int asn_MAP_ASN_RRC_powerControlOffsetSS_enum2value_5[] = {
	0,	/* db-3(0) */
	1,	/* db0(1) */
	2,	/* db3(2) */
	3	/* db6(3) */
};
static const asn_INTEGER_specifics_t asn_SPC_ASN_RRC_powerControlOffsetSS_specs_5 = {
	asn_MAP_ASN_RRC_powerControlOffsetSS_value2enum_5,	/* "tag" => N; sorted by tag */
	asn_MAP_ASN_RRC_powerControlOffsetSS_enum2value_5,	/* N => "tag"; sorted by N */
	4,	/* Number of elements in the maps */
	0,	/* Enumeration is not extensible */
	1,	/* Strict enumeration */
	0,	/* Native long size */
	0
};
static const ber_tlv_tag_t asn_DEF_ASN_RRC_powerControlOffsetSS_tags_5[] = {
	(ASN_TAG_CLASS_CONTEXT | (3 << 2)),
	(ASN_TAG_CLASS_UNIVERSAL | (10 << 2))
};
static /* Use -fall-defs-global to expose */
asn_TYPE_descriptor_t asn_DEF_ASN_RRC_powerControlOffsetSS_5 = {
	"powerControlOffsetSS",
	"powerControlOffsetSS",
	&asn_OP_NativeEnumerated,
	asn_DEF_ASN_RRC_powerControlOffsetSS_tags_5,
	sizeof(asn_DEF_ASN_RRC_powerControlOffsetSS_tags_5)
		/sizeof(asn_DEF_ASN_RRC_powerControlOffsetSS_tags_5[0]) - 1, /* 1 */
	asn_DEF_ASN_RRC_powerControlOffsetSS_tags_5,	/* Same as above */
	sizeof(asn_DEF_ASN_RRC_powerControlOffsetSS_tags_5)
		/sizeof(asn_DEF_ASN_RRC_powerControlOffsetSS_tags_5[0]), /* 2 */
	{ 0, &asn_PER_type_ASN_RRC_powerControlOffsetSS_constr_5, NativeEnumerated_constraint },
	0, 0,	/* Defined elsewhere */
	&asn_SPC_ASN_RRC_powerControlOffsetSS_specs_5	/* Additional specs */
};

asn_TYPE_member_t asn_MBR_ASN_RRC_NZP_CSI_RS_Resource_1[] = {
	{ ATF_NOFLAGS, 0, offsetof(struct ASN_RRC_NZP_CSI_RS_Resource, nzp_CSI_RS_ResourceId),
		(ASN_TAG_CLASS_CONTEXT | (0 << 2)),
		-1,	/* IMPLICIT tag at current level */
		&asn_DEF_ASN_RRC_NZP_CSI_RS_ResourceId,
		0,
		{ 0, 0, 0 },
		0, 0, /* No default value */
		"nzp-CSI-RS-ResourceId"
		},
	{ ATF_NOFLAGS, 0, offsetof(struct ASN_RRC_NZP_CSI_RS_Resource, resourceMapping),
		(ASN_TAG_CLASS_CONTEXT | (1 << 2)),
		-1,	/* IMPLICIT tag at current level */
		&asn_DEF_ASN_RRC_CSI_RS_ResourceMapping,
		0,
		{ 0, 0, 0 },
		0, 0, /* No default value */
		"resourceMapping"
		},
	{ ATF_NOFLAGS, 0, offsetof(struct ASN_RRC_NZP_CSI_RS_Resource, powerControlOffset),
		(ASN_TAG_CLASS_CONTEXT | (2 << 2)),
		-1,	/* IMPLICIT tag at current level */
		&asn_DEF_NativeInteger,
		0,
		{ 0, &asn_PER_memb_ASN_RRC_powerControlOffset_constr_4,  memb_ASN_RRC_powerControlOffset_constraint_1 },
		0, 0, /* No default value */
		"powerControlOffset"
		},
	{ ATF_POINTER, 1, offsetof(struct ASN_RRC_NZP_CSI_RS_Resource, powerControlOffsetSS),
		(ASN_TAG_CLASS_CONTEXT | (3 << 2)),
		-1,	/* IMPLICIT tag at current level */
		&asn_DEF_ASN_RRC_powerControlOffsetSS_5,
		0,
		{ 0, 0, 0 },
		0, 0, /* No default value */
		"powerControlOffsetSS"
		},
	{ ATF_NOFLAGS, 0, offsetof(struct ASN_RRC_NZP_CSI_RS_Resource, scramblingID),
		(ASN_TAG_CLASS_CONTEXT | (4 << 2)),
		-1,	/* IMPLICIT tag at current level */
		&asn_DEF_ASN_RRC_ScramblingId,
		0,
		{ 0, 0, 0 },
		0, 0, /* No default value */
		"scramblingID"
		},
	{ ATF_POINTER, 2, offsetof(struct ASN_RRC_NZP_CSI_RS_Resource, periodicityAndOffset),
		(ASN_TAG_CLASS_CONTEXT | (5 << 2)),
		+1,	/* EXPLICIT tag at current level */
		&asn_DEF_ASN_RRC_CSI_ResourcePeriodicityAndOffset,
		0,
		{ 0, 0, 0 },
		0, 0, /* No default value */
		"periodicityAndOffset"
		},
	{ ATF_POINTER, 1, offsetof(struct ASN_RRC_NZP_CSI_RS_Resource, qcl_InfoPeriodicCSI_RS),
		(ASN_TAG_CLASS_CONTEXT | (6 << 2)),
		-1,	/* IMPLICIT tag at current level */
		&asn_DEF_ASN_RRC_TCI_StateId,
		0,
		{ 0, 0, 0 },
		0, 0, /* No default value */
		"qcl-InfoPeriodicCSI-RS"
		},
};
static const int asn_MAP_ASN_RRC_NZP_CSI_RS_Resource_oms_1[] = { 3, 5, 6 };
static const ber_tlv_tag_t asn_DEF_ASN_RRC_NZP_CSI_RS_Resource_tags_1[] = {
	(ASN_TAG_CLASS_UNIVERSAL | (16 << 2))
};
static const asn_TYPE_tag2member_t asn_MAP_ASN_RRC_NZP_CSI_RS_Resource_tag2el_1[] = {
    { (ASN_TAG_CLASS_CONTEXT | (0 << 2)), 0, 0, 0 }, /* nzp-CSI-RS-ResourceId */
    { (ASN_TAG_CLASS_CONTEXT | (1 << 2)), 1, 0, 0 }, /* resourceMapping */
    { (ASN_TAG_CLASS_CONTEXT | (2 << 2)), 2, 0, 0 }, /* powerControlOffset */
    { (ASN_TAG_CLASS_CONTEXT | (3 << 2)), 3, 0, 0 }, /* powerControlOffsetSS */
    { (ASN_TAG_CLASS_CONTEXT | (4 << 2)), 4, 0, 0 }, /* scramblingID */
    { (ASN_TAG_CLASS_CONTEXT | (5 << 2)), 5, 0, 0 }, /* periodicityAndOffset */
    { (ASN_TAG_CLASS_CONTEXT | (6 << 2)), 6, 0, 0 } /* qcl-InfoPeriodicCSI-RS */
};
asn_SEQUENCE_specifics_t asn_SPC_ASN_RRC_NZP_CSI_RS_Resource_specs_1 = {
	sizeof(struct ASN_RRC_NZP_CSI_RS_Resource),
	offsetof(struct ASN_RRC_NZP_CSI_RS_Resource, _asn_ctx),
	asn_MAP_ASN_RRC_NZP_CSI_RS_Resource_tag2el_1,
	7,	/* Count of tags in the map */
	asn_MAP_ASN_RRC_NZP_CSI_RS_Resource_oms_1,	/* Optional members */
	3, 0,	/* Root/Additions */
	7,	/* First extension addition */
};
asn_TYPE_descriptor_t asn_DEF_ASN_RRC_NZP_CSI_RS_Resource = {
	"NZP-CSI-RS-Resource",
	"NZP-CSI-RS-Resource",
	&asn_OP_SEQUENCE,
	asn_DEF_ASN_RRC_NZP_CSI_RS_Resource_tags_1,
	sizeof(asn_DEF_ASN_RRC_NZP_CSI_RS_Resource_tags_1)
		/sizeof(asn_DEF_ASN_RRC_NZP_CSI_RS_Resource_tags_1[0]), /* 1 */
	asn_DEF_ASN_RRC_NZP_CSI_RS_Resource_tags_1,	/* Same as above */
	sizeof(asn_DEF_ASN_RRC_NZP_CSI_RS_Resource_tags_1)
		/sizeof(asn_DEF_ASN_RRC_NZP_CSI_RS_Resource_tags_1[0]), /* 1 */
	{ 0, 0, SEQUENCE_constraint },
	asn_MBR_ASN_RRC_NZP_CSI_RS_Resource_1,
	7,	/* Elements count */
	&asn_SPC_ASN_RRC_NZP_CSI_RS_Resource_specs_1	/* Additional specs */
};

