/*
 * Generated by asn1c-0.9.29 (http://lionet.info/asn1c)
 * From ASN.1 module "NR-RRC-Definitions"
 * 	found in "asn/nr-rrc-15.6.0.asn1"
 * 	`asn1c -fcompound-names -pdu=all -findirect-choice -fno-include-deps -gen-PER -no-gen-OER -no-gen-example -D rrc`
 */

#ifndef	_ASN_RRC_DummyB_H_
#define	_ASN_RRC_DummyB_H_


#include <asn_application.h>

/* Including external dependencies */
#include <NativeEnumerated.h>
#include <NativeInteger.h>
#include <constr_SEQUENCE.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Dependencies */
typedef enum ASN_RRC_DummyB__maxNumberTxPortsPerResource {
	ASN_RRC_DummyB__maxNumberTxPortsPerResource_p2	= 0,
	ASN_RRC_DummyB__maxNumberTxPortsPerResource_p4	= 1,
	ASN_RRC_DummyB__maxNumberTxPortsPerResource_p8	= 2,
	ASN_RRC_DummyB__maxNumberTxPortsPerResource_p12	= 3,
	ASN_RRC_DummyB__maxNumberTxPortsPerResource_p16	= 4,
	ASN_RRC_DummyB__maxNumberTxPortsPerResource_p24	= 5,
	ASN_RRC_DummyB__maxNumberTxPortsPerResource_p32	= 6
} e_ASN_RRC_DummyB__maxNumberTxPortsPerResource;
typedef enum ASN_RRC_DummyB__supportedCodebookMode {
	ASN_RRC_DummyB__supportedCodebookMode_mode1	= 0,
	ASN_RRC_DummyB__supportedCodebookMode_mode1AndMode2	= 1
} e_ASN_RRC_DummyB__supportedCodebookMode;

/* ASN_RRC_DummyB */
typedef struct ASN_RRC_DummyB {
	long	 maxNumberTxPortsPerResource;
	long	 maxNumberResources;
	long	 totalNumberTxPorts;
	long	 supportedCodebookMode;
	long	 maxNumberCSI_RS_PerResourceSet;
	
	/* Context for parsing across buffer boundaries */
	asn_struct_ctx_t _asn_ctx;
} ASN_RRC_DummyB_t;

/* Implementation */
/* extern asn_TYPE_descriptor_t asn_DEF_ASN_RRC_maxNumberTxPortsPerResource_2;	// (Use -fall-defs-global to expose) */
/* extern asn_TYPE_descriptor_t asn_DEF_ASN_RRC_supportedCodebookMode_12;	// (Use -fall-defs-global to expose) */
extern asn_TYPE_descriptor_t asn_DEF_ASN_RRC_DummyB;
extern asn_SEQUENCE_specifics_t asn_SPC_ASN_RRC_DummyB_specs_1;
extern asn_TYPE_member_t asn_MBR_ASN_RRC_DummyB_1[5];

#ifdef __cplusplus
}
#endif

#endif	/* _ASN_RRC_DummyB_H_ */
#include <asn_internal.h>
