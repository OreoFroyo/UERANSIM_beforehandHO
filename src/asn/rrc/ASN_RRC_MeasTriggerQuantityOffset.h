/*
 * Generated by asn1c-0.9.29 (http://lionet.info/asn1c)
 * From ASN.1 module "NR-RRC-Definitions"
 * 	found in "asn/nr-rrc-15.6.0.asn1"
 * 	`asn1c -fcompound-names -pdu=all -findirect-choice -fno-include-deps -gen-PER -no-gen-OER -no-gen-example -D rrc`
 */

#ifndef	_ASN_RRC_MeasTriggerQuantityOffset_H_
#define	_ASN_RRC_MeasTriggerQuantityOffset_H_


#include <asn_application.h>

/* Including external dependencies */
#include <NativeInteger.h>
#include <constr_CHOICE.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Dependencies */
typedef enum ASN_RRC_MeasTriggerQuantityOffset_PR {
	ASN_RRC_MeasTriggerQuantityOffset_PR_NOTHING,	/* No components present */
	ASN_RRC_MeasTriggerQuantityOffset_PR_rsrp,
	ASN_RRC_MeasTriggerQuantityOffset_PR_rsrq,
	ASN_RRC_MeasTriggerQuantityOffset_PR_sinr
} ASN_RRC_MeasTriggerQuantityOffset_PR;

/* ASN_RRC_MeasTriggerQuantityOffset */
typedef struct ASN_RRC_MeasTriggerQuantityOffset {
	ASN_RRC_MeasTriggerQuantityOffset_PR present;
	union ASN_RRC_MeasTriggerQuantityOffset_u {
		long	 rsrp;
		long	 rsrq;
		long	 sinr;
	} choice;
	
	/* Context for parsing across buffer boundaries */
	asn_struct_ctx_t _asn_ctx;
} ASN_RRC_MeasTriggerQuantityOffset_t;

/* Implementation */
extern asn_TYPE_descriptor_t asn_DEF_ASN_RRC_MeasTriggerQuantityOffset;
extern asn_CHOICE_specifics_t asn_SPC_ASN_RRC_MeasTriggerQuantityOffset_specs_1;
extern asn_TYPE_member_t asn_MBR_ASN_RRC_MeasTriggerQuantityOffset_1[3];
extern asn_per_constraints_t asn_PER_type_ASN_RRC_MeasTriggerQuantityOffset_constr_1;

#ifdef __cplusplus
}
#endif

#endif	/* _ASN_RRC_MeasTriggerQuantityOffset_H_ */
#include <asn_internal.h>
