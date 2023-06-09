/*
 * Generated by asn1c-0.9.29 (http://lionet.info/asn1c)
 * From ASN.1 module "NR-RRC-Definitions"
 * 	found in "asn/nr-rrc-15.6.0.asn1"
 * 	`asn1c -fcompound-names -pdu=all -findirect-choice -fno-include-deps -gen-PER -no-gen-OER -no-gen-example -D rrc`
 */

#include "ASN_RRC_UE-NR-Capability.h"

#include "ASN_RRC_RLC-Parameters.h"
#include "ASN_RRC_MAC-Parameters.h"
#include "ASN_RRC_MeasAndMobParameters.h"
#include "ASN_RRC_UE-NR-CapabilityAddXDD-Mode.h"
#include "ASN_RRC_UE-NR-CapabilityAddFRX-Mode.h"
#include "ASN_RRC_FeatureSets.h"
#include "ASN_RRC_UE-NR-Capability-v1530.h"
#include "ASN_RRC_FeatureSetCombination.h"
static int
memb_ASN_RRC_featureSetCombinations_constraint_1(const asn_TYPE_descriptor_t *td, const void *sptr,
			asn_app_constraint_failed_f *ctfailcb, void *app_key) {
	size_t size;
	
	if(!sptr) {
		ASN__CTFAIL(app_key, td, sptr,
			"%s: value not given (%s:%d)",
			td->name, __FILE__, __LINE__);
		return -1;
	}
	
	/* Determine the number of elements */
	size = _A_CSEQUENCE_FROM_VOID(sptr)->count;
	
	if((size >= 1 && size <= 1024)) {
		/* Perform validation of the inner elements */
		return td->encoding_constraints.general_constraints(td, sptr, ctfailcb, app_key);
	} else {
		ASN__CTFAIL(app_key, td, sptr,
			"%s: constraint failed (%s:%d)",
			td->name, __FILE__, __LINE__);
		return -1;
	}
}

static asn_per_constraints_t asn_PER_type_ASN_RRC_featureSetCombinations_constr_14 CC_NOTUSED = {
	{ APC_UNCONSTRAINED,	-1, -1,  0,  0 },
	{ APC_CONSTRAINED,	 10,  10,  1,  1024 }	/* (SIZE(1..1024)) */,
	0, 0	/* No PER value map */
};
static asn_per_constraints_t asn_PER_memb_ASN_RRC_featureSetCombinations_constr_14 CC_NOTUSED = {
	{ APC_UNCONSTRAINED,	-1, -1,  0,  0 },
	{ APC_CONSTRAINED,	 10,  10,  1,  1024 }	/* (SIZE(1..1024)) */,
	0, 0	/* No PER value map */
};
static asn_TYPE_member_t asn_MBR_ASN_RRC_featureSetCombinations_14[] = {
	{ ATF_POINTER, 0, 0,
		(ASN_TAG_CLASS_UNIVERSAL | (16 << 2)),
		0,
		&asn_DEF_ASN_RRC_FeatureSetCombination,
		0,
		{ 0, 0, 0 },
		0, 0, /* No default value */
		""
		},
};
static const ber_tlv_tag_t asn_DEF_ASN_RRC_featureSetCombinations_tags_14[] = {
	(ASN_TAG_CLASS_CONTEXT | (12 << 2)),
	(ASN_TAG_CLASS_UNIVERSAL | (16 << 2))
};
static asn_SET_OF_specifics_t asn_SPC_ASN_RRC_featureSetCombinations_specs_14 = {
	sizeof(struct ASN_RRC_UE_NR_Capability__featureSetCombinations),
	offsetof(struct ASN_RRC_UE_NR_Capability__featureSetCombinations, _asn_ctx),
	0,	/* XER encoding is XMLDelimitedItemList */
};
static /* Use -fall-defs-global to expose */
asn_TYPE_descriptor_t asn_DEF_ASN_RRC_featureSetCombinations_14 = {
	"featureSetCombinations",
	"featureSetCombinations",
	&asn_OP_SEQUENCE_OF,
	asn_DEF_ASN_RRC_featureSetCombinations_tags_14,
	sizeof(asn_DEF_ASN_RRC_featureSetCombinations_tags_14)
		/sizeof(asn_DEF_ASN_RRC_featureSetCombinations_tags_14[0]) - 1, /* 1 */
	asn_DEF_ASN_RRC_featureSetCombinations_tags_14,	/* Same as above */
	sizeof(asn_DEF_ASN_RRC_featureSetCombinations_tags_14)
		/sizeof(asn_DEF_ASN_RRC_featureSetCombinations_tags_14[0]), /* 2 */
	{ 0, &asn_PER_type_ASN_RRC_featureSetCombinations_constr_14, SEQUENCE_OF_constraint },
	asn_MBR_ASN_RRC_featureSetCombinations_14,
	1,	/* Single element */
	&asn_SPC_ASN_RRC_featureSetCombinations_specs_14	/* Additional specs */
};

static asn_TYPE_member_t asn_MBR_ASN_RRC_UE_NR_Capability_1[] = {
	{ ATF_NOFLAGS, 0, offsetof(struct ASN_RRC_UE_NR_Capability, accessStratumRelease),
		(ASN_TAG_CLASS_CONTEXT | (0 << 2)),
		-1,	/* IMPLICIT tag at current level */
		&asn_DEF_ASN_RRC_AccessStratumRelease,
		0,
		{ 0, 0, 0 },
		0, 0, /* No default value */
		"accessStratumRelease"
		},
	{ ATF_NOFLAGS, 0, offsetof(struct ASN_RRC_UE_NR_Capability, pdcp_Parameters),
		(ASN_TAG_CLASS_CONTEXT | (1 << 2)),
		-1,	/* IMPLICIT tag at current level */
		&asn_DEF_ASN_RRC_PDCP_Parameters,
		0,
		{ 0, 0, 0 },
		0, 0, /* No default value */
		"pdcp-Parameters"
		},
	{ ATF_POINTER, 2, offsetof(struct ASN_RRC_UE_NR_Capability, rlc_Parameters),
		(ASN_TAG_CLASS_CONTEXT | (2 << 2)),
		-1,	/* IMPLICIT tag at current level */
		&asn_DEF_ASN_RRC_RLC_Parameters,
		0,
		{ 0, 0, 0 },
		0, 0, /* No default value */
		"rlc-Parameters"
		},
	{ ATF_POINTER, 1, offsetof(struct ASN_RRC_UE_NR_Capability, mac_Parameters),
		(ASN_TAG_CLASS_CONTEXT | (3 << 2)),
		-1,	/* IMPLICIT tag at current level */
		&asn_DEF_ASN_RRC_MAC_Parameters,
		0,
		{ 0, 0, 0 },
		0, 0, /* No default value */
		"mac-Parameters"
		},
	{ ATF_NOFLAGS, 0, offsetof(struct ASN_RRC_UE_NR_Capability, phy_Parameters),
		(ASN_TAG_CLASS_CONTEXT | (4 << 2)),
		-1,	/* IMPLICIT tag at current level */
		&asn_DEF_ASN_RRC_Phy_Parameters,
		0,
		{ 0, 0, 0 },
		0, 0, /* No default value */
		"phy-Parameters"
		},
	{ ATF_NOFLAGS, 0, offsetof(struct ASN_RRC_UE_NR_Capability, rf_Parameters),
		(ASN_TAG_CLASS_CONTEXT | (5 << 2)),
		-1,	/* IMPLICIT tag at current level */
		&asn_DEF_ASN_RRC_RF_Parameters,
		0,
		{ 0, 0, 0 },
		0, 0, /* No default value */
		"rf-Parameters"
		},
	{ ATF_POINTER, 9, offsetof(struct ASN_RRC_UE_NR_Capability, measAndMobParameters),
		(ASN_TAG_CLASS_CONTEXT | (6 << 2)),
		-1,	/* IMPLICIT tag at current level */
		&asn_DEF_ASN_RRC_MeasAndMobParameters,
		0,
		{ 0, 0, 0 },
		0, 0, /* No default value */
		"measAndMobParameters"
		},
	{ ATF_POINTER, 8, offsetof(struct ASN_RRC_UE_NR_Capability, fdd_Add_UE_NR_Capabilities),
		(ASN_TAG_CLASS_CONTEXT | (7 << 2)),
		-1,	/* IMPLICIT tag at current level */
		&asn_DEF_ASN_RRC_UE_NR_CapabilityAddXDD_Mode,
		0,
		{ 0, 0, 0 },
		0, 0, /* No default value */
		"fdd-Add-UE-NR-Capabilities"
		},
	{ ATF_POINTER, 7, offsetof(struct ASN_RRC_UE_NR_Capability, tdd_Add_UE_NR_Capabilities),
		(ASN_TAG_CLASS_CONTEXT | (8 << 2)),
		-1,	/* IMPLICIT tag at current level */
		&asn_DEF_ASN_RRC_UE_NR_CapabilityAddXDD_Mode,
		0,
		{ 0, 0, 0 },
		0, 0, /* No default value */
		"tdd-Add-UE-NR-Capabilities"
		},
	{ ATF_POINTER, 6, offsetof(struct ASN_RRC_UE_NR_Capability, fr1_Add_UE_NR_Capabilities),
		(ASN_TAG_CLASS_CONTEXT | (9 << 2)),
		-1,	/* IMPLICIT tag at current level */
		&asn_DEF_ASN_RRC_UE_NR_CapabilityAddFRX_Mode,
		0,
		{ 0, 0, 0 },
		0, 0, /* No default value */
		"fr1-Add-UE-NR-Capabilities"
		},
	{ ATF_POINTER, 5, offsetof(struct ASN_RRC_UE_NR_Capability, fr2_Add_UE_NR_Capabilities),
		(ASN_TAG_CLASS_CONTEXT | (10 << 2)),
		-1,	/* IMPLICIT tag at current level */
		&asn_DEF_ASN_RRC_UE_NR_CapabilityAddFRX_Mode,
		0,
		{ 0, 0, 0 },
		0, 0, /* No default value */
		"fr2-Add-UE-NR-Capabilities"
		},
	{ ATF_POINTER, 4, offsetof(struct ASN_RRC_UE_NR_Capability, featureSets),
		(ASN_TAG_CLASS_CONTEXT | (11 << 2)),
		-1,	/* IMPLICIT tag at current level */
		&asn_DEF_ASN_RRC_FeatureSets,
		0,
		{ 0, 0, 0 },
		0, 0, /* No default value */
		"featureSets"
		},
	{ ATF_POINTER, 3, offsetof(struct ASN_RRC_UE_NR_Capability, featureSetCombinations),
		(ASN_TAG_CLASS_CONTEXT | (12 << 2)),
		0,
		&asn_DEF_ASN_RRC_featureSetCombinations_14,
		0,
		{ 0, &asn_PER_memb_ASN_RRC_featureSetCombinations_constr_14,  memb_ASN_RRC_featureSetCombinations_constraint_1 },
		0, 0, /* No default value */
		"featureSetCombinations"
		},
	{ ATF_POINTER, 2, offsetof(struct ASN_RRC_UE_NR_Capability, lateNonCriticalExtension),
		(ASN_TAG_CLASS_CONTEXT | (13 << 2)),
		-1,	/* IMPLICIT tag at current level */
		&asn_DEF_OCTET_STRING,
		0,
		{ 0, 0, 0 },
		0, 0, /* No default value */
		"lateNonCriticalExtension"
		},
	{ ATF_POINTER, 1, offsetof(struct ASN_RRC_UE_NR_Capability, nonCriticalExtension),
		(ASN_TAG_CLASS_CONTEXT | (14 << 2)),
		-1,	/* IMPLICIT tag at current level */
		&asn_DEF_ASN_RRC_UE_NR_Capability_v1530,
		0,
		{ 0, 0, 0 },
		0, 0, /* No default value */
		"nonCriticalExtension"
		},
};
static const int asn_MAP_ASN_RRC_UE_NR_Capability_oms_1[] = { 2, 3, 6, 7, 8, 9, 10, 11, 12, 13, 14 };
static const ber_tlv_tag_t asn_DEF_ASN_RRC_UE_NR_Capability_tags_1[] = {
	(ASN_TAG_CLASS_UNIVERSAL | (16 << 2))
};
static const asn_TYPE_tag2member_t asn_MAP_ASN_RRC_UE_NR_Capability_tag2el_1[] = {
    { (ASN_TAG_CLASS_CONTEXT | (0 << 2)), 0, 0, 0 }, /* accessStratumRelease */
    { (ASN_TAG_CLASS_CONTEXT | (1 << 2)), 1, 0, 0 }, /* pdcp-Parameters */
    { (ASN_TAG_CLASS_CONTEXT | (2 << 2)), 2, 0, 0 }, /* rlc-Parameters */
    { (ASN_TAG_CLASS_CONTEXT | (3 << 2)), 3, 0, 0 }, /* mac-Parameters */
    { (ASN_TAG_CLASS_CONTEXT | (4 << 2)), 4, 0, 0 }, /* phy-Parameters */
    { (ASN_TAG_CLASS_CONTEXT | (5 << 2)), 5, 0, 0 }, /* rf-Parameters */
    { (ASN_TAG_CLASS_CONTEXT | (6 << 2)), 6, 0, 0 }, /* measAndMobParameters */
    { (ASN_TAG_CLASS_CONTEXT | (7 << 2)), 7, 0, 0 }, /* fdd-Add-UE-NR-Capabilities */
    { (ASN_TAG_CLASS_CONTEXT | (8 << 2)), 8, 0, 0 }, /* tdd-Add-UE-NR-Capabilities */
    { (ASN_TAG_CLASS_CONTEXT | (9 << 2)), 9, 0, 0 }, /* fr1-Add-UE-NR-Capabilities */
    { (ASN_TAG_CLASS_CONTEXT | (10 << 2)), 10, 0, 0 }, /* fr2-Add-UE-NR-Capabilities */
    { (ASN_TAG_CLASS_CONTEXT | (11 << 2)), 11, 0, 0 }, /* featureSets */
    { (ASN_TAG_CLASS_CONTEXT | (12 << 2)), 12, 0, 0 }, /* featureSetCombinations */
    { (ASN_TAG_CLASS_CONTEXT | (13 << 2)), 13, 0, 0 }, /* lateNonCriticalExtension */
    { (ASN_TAG_CLASS_CONTEXT | (14 << 2)), 14, 0, 0 } /* nonCriticalExtension */
};
static asn_SEQUENCE_specifics_t asn_SPC_ASN_RRC_UE_NR_Capability_specs_1 = {
	sizeof(struct ASN_RRC_UE_NR_Capability),
	offsetof(struct ASN_RRC_UE_NR_Capability, _asn_ctx),
	asn_MAP_ASN_RRC_UE_NR_Capability_tag2el_1,
	15,	/* Count of tags in the map */
	asn_MAP_ASN_RRC_UE_NR_Capability_oms_1,	/* Optional members */
	11, 0,	/* Root/Additions */
	-1,	/* First extension addition */
};
asn_TYPE_descriptor_t asn_DEF_ASN_RRC_UE_NR_Capability = {
	"UE-NR-Capability",
	"UE-NR-Capability",
	&asn_OP_SEQUENCE,
	asn_DEF_ASN_RRC_UE_NR_Capability_tags_1,
	sizeof(asn_DEF_ASN_RRC_UE_NR_Capability_tags_1)
		/sizeof(asn_DEF_ASN_RRC_UE_NR_Capability_tags_1[0]), /* 1 */
	asn_DEF_ASN_RRC_UE_NR_Capability_tags_1,	/* Same as above */
	sizeof(asn_DEF_ASN_RRC_UE_NR_Capability_tags_1)
		/sizeof(asn_DEF_ASN_RRC_UE_NR_Capability_tags_1[0]), /* 1 */
	{ 0, 0, SEQUENCE_constraint },
	asn_MBR_ASN_RRC_UE_NR_Capability_1,
	15,	/* Elements count */
	&asn_SPC_ASN_RRC_UE_NR_Capability_specs_1	/* Additional specs */
};

