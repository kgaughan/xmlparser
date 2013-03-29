/*                                       vim:set ts=4 sw=4 noai sr sta et cin:
 * XMLParser.cpp
 * by Keith Gaughan <kmgaughan@eircom.net>
 *
 * A C++ wrapper around the Expat parser library.
 *
 * Copyright (c) Keith Gaughan, 2004.
 * All Rights Reserved.
 *
 * Permission is granted to anyone to use this software for any purpose on any
 * computer system, and to alter it and redistribute it, subject to the
 * following restrictions:
 *
 *  1. The author is not responsible for the consequences of use of this
 *     software, no matter how awful, even if they arise from flaws in it.
 *
 *  2. The origin of this software must not be misrepresented, either by
 *     explicit claim or by omission. Since few users ever read sources,
 *     credits must appear in the documentation.
 *
 *  3. Altered versions must be plainly marked as such, and must not be
 *     misrepresented as being the original software. Since few users ever
 *     read sources, credits must appear in the documentation.
 *
 *  4. The author reserves the right to change the licencing details on any
 *     future releases of this package.
 *
 *  5. This notice may not be removed or altered.
 */

#include "XMLParser.h"
#include "expat.h"
#include <assert.h>
#include <string.h>

/********************************************** Internal Parser Callbacks **/

extern "C" {

static void StartElementHandler(void* pUserData, const XML_Char* name, const XML_Char** attrs)
{
	XMLEvtHandler* ph = static_cast<XMLEvtHandler*>(pUserData);
	ph->OnStartElement(name, attrs);
}

static void EndElementHandler(void* pUserData, const XML_Char* name)
{
	XMLEvtHandler* ph = static_cast<XMLEvtHandler*>(pUserData);
	ph->OnEndElement(name);
}

static void CharacterDataHandler(void* pUserData, const XML_Char* data, int len)
{
	XMLEvtHandler* ph = static_cast<XMLEvtHandler*>(pUserData);
	ph->OnCharacterData(data, len);
}

static void ProcessingInstructionHandler(void* pUserData, const XML_Char* target, const XML_Char* data)
{
	XMLEvtHandler* ph = static_cast<XMLEvtHandler*>(pUserData);
	ph->OnProcessingInstruction(target, data);
}

static void CommentHandler(void* pUserData, const XML_Char* data)
{
	XMLEvtHandler* ph = static_cast<XMLEvtHandler*>(pUserData);
	ph->OnComment(data);
}

static void StartCdataSectionHandler(void* pUserData)
{
	XMLEvtHandler* ph = static_cast<XMLEvtHandler*>(pUserData);
	ph->OnStartCdataSection();
}

static void EndCdataSectionHandler(void* pUserData)
{
	XMLEvtHandler* ph = static_cast<XMLEvtHandler*>(pUserData);
	ph->OnEndCdataSection();
}

static void DefaultHandler(void* pUserData, const XML_Char* data, int len)
{
	XMLEvtHandler* ph = static_cast<XMLEvtHandler*>(pUserData);
	ph->OnDefault(data, len);
}

static int UnknownEncodingHandler(void* pUserData, const XML_Char* name, XML_Encoding* pInfo)
{
	XMLEvtHandler* ph = static_cast<XMLEvtHandler*>(pUserData);
	return ph->OnUnknownEncoding(name, pInfo) ? 1 : 0;
}

static void StartNamespaceDeclHandler(void* pUserData, const XML_Char* prefix, const XML_Char* uri)
{
	XMLEvtHandler* ph = static_cast<XMLEvtHandler*>(pUserData);
	ph->OnStartNamespaceDecl(prefix, uri);
}

static void EndNamespaceDeclHandler(void* pUserData, const XML_Char* prefix)
{
	XMLEvtHandler* ph = static_cast<XMLEvtHandler*>(pUserData);
	ph->OnEndNamespaceDecl(prefix);
}

static void XmlDeclHandler(void* pUserData, const XML_Char* version, const XML_Char* encoding, int isStandalone)
{
	XMLEvtHandler* ph = static_cast<XMLEvtHandler*>(pUserData);
	ph->OnXmlDecl(version, encoding, isStandalone != 0);
}

static void StartDoctypeDeclHandler(void* pUserData, const XML_Char* doctype, const XML_Char* sysId, const XML_Char* pubId, int hasInternalSubset)
{
	XMLEvtHandler* ph = static_cast<XMLEvtHandler*>(pUserData);
	ph->OnStartDoctypeDecl(doctype, sysId, pubId, hasInternalSubset != 0);
}

static void EndDoctypeDeclHandler(void* pUserData)
{
	XMLEvtHandler* ph = static_cast<XMLEvtHandler*>(pUserData);
	ph->OnEndDoctypeDecl();
}

};

/****************************************** Parser Wrapper Implementation **/

XMLParser::XMLParser(XMLEvtHandler* pHandler)
{
	m_parser   = NULL;
	m_pHandler = pHandler;
}

XMLParser::~XMLParser()
{
	Destroy();
}

bool XMLParser::Create(const XML_Char* encoding, const XML_Char* sep)
{
	// Destroy the old parser.
	Destroy();

	// If the encoding or seperator are empty, then NULL.
	if (encoding != NULL && *encoding == '\0')
		encoding = NULL;
	if (sep != NULL && *sep == '\0')
		sep = NULL;

	// Create the new one, if possible, and initialise.
	m_parser = XML_ParserCreate_MM(encoding, NULL, sep);
	if (m_parser == NULL)
		return false;
	m_pHandler->OnPostCreate();

	// Set the user data used in callbacks.
	XML_SetUserData(m_parser, (void*) m_pHandler);
	return true;
}

void XMLParser::Destroy()
{
	if (m_parser != NULL)
		XML_ParserFree(m_parser);
	m_parser = NULL;
}

bool XMLParser::Parse(const char* buf, size_t len, bool isFinal)
{
	assert(m_parser != NULL);
	return XML_Parse(m_parser, buf, len, isFinal) != 0;
}

bool XMLParser::Parse(const char* buf, bool isFinal)
{
	return Parse(buf, strlen(buf), isFinal);
}

bool XMLParser::Parse(size_t len, bool isFinal)
{
	assert(m_parser != NULL);
	return XML_ParseBuffer(m_parser, len, isFinal) != 0;
}

void* XMLParser::GetBuffer(size_t len)
{
	assert(m_parser != NULL);
	return XML_GetBuffer(m_parser, len);
}

/******************************** Parser callback enable/disable methods **/

void XMLParser::EnableStartElementHandler(bool enable)
{
	assert(m_parser != NULL);
	XML_SetStartElementHandler(m_parser, enable ? StartElementHandler : NULL);
}

void XMLParser::EnableEndElementHandler(bool enable)
{
	assert(m_parser != NULL);
	XML_SetEndElementHandler(m_parser, enable ? EndElementHandler : NULL);
}

void XMLParser::EnableElementHandler(bool enable)
{
	assert(m_parser != NULL);
	EnableStartElementHandler(enable);
	EnableEndElementHandler(enable);
}

void XMLParser::EnableCharacterDataHandler(bool enable)
{
	assert(m_parser != NULL);
	XML_SetCharacterDataHandler(m_parser, enable ? CharacterDataHandler : NULL);
}

void XMLParser::EnableProcessingInstructionHandler(bool enable)
{
	assert(m_parser != NULL);
	XML_SetProcessingInstructionHandler(m_parser,
	enable ? ProcessingInstructionHandler : NULL);
}

void XMLParser::EnableCommentHandler(bool enable)
{
	assert(m_parser != NULL);
	XML_SetCommentHandler(m_parser, enable ? CommentHandler : NULL);
}

void XMLParser::EnableStartCdataSectionHandler(bool enable)
{
	assert(m_parser != NULL);
	XML_SetStartCdataSectionHandler(m_parser, enable ? StartCdataSectionHandler : NULL);
}

void XMLParser::EnableEndCdataSectionHandler(bool enable)
{
	assert(m_parser != NULL);
	XML_SetEndCdataSectionHandler(m_parser, enable ? EndCdataSectionHandler : NULL);
}

void XMLParser::EnableCdataSectionHandler(bool enable)
{
	assert(m_parser != NULL);
	EnableStartCdataSectionHandler(enable);
	EnableEndCdataSectionHandler(enable);
}

void XMLParser::EnableDefaultHandler(bool enable, bool expand)
{
	assert(m_parser != NULL);
	if (expand)
		XML_SetDefaultHandlerExpand(m_parser, enable ? DefaultHandler : NULL);
	else
		XML_SetDefaultHandler(m_parser, enable ? DefaultHandler : NULL);
}

void XMLParser::EnableUnknownEncodingHandler(bool enable)
{
	assert(m_parser != NULL);
	XML_SetUnknownEncodingHandler(m_parser, enable ? UnknownEncodingHandler : NULL, NULL);
}

void XMLParser::EnableStartNamespaceDeclHandler(bool enable)
{
	assert(m_parser != NULL);
	XML_SetStartNamespaceDeclHandler(m_parser, enable ? StartNamespaceDeclHandler : NULL);
}

void XMLParser::EnableEndNamespaceDeclHandler(bool enable)
{
	assert(m_parser != NULL);
	XML_SetEndNamespaceDeclHandler(m_parser, enable ? EndNamespaceDeclHandler : NULL);
}

void XMLParser::EnableNamespaceDeclHandler(bool enable)
{
	EnableStartNamespaceDeclHandler(enable);
	EnableEndNamespaceDeclHandler(enable);
}

void XMLParser::EnableXmlDeclHandler(bool enable)
{
	assert(m_parser != NULL);
	XML_SetXmlDeclHandler(m_parser, enable ? XmlDeclHandler : NULL);
}

void XMLParser::EnableStartDoctypeDeclHandler(bool enable)
{
	assert(m_parser != NULL);
	XML_SetStartDoctypeDeclHandler(m_parser, enable ? StartDoctypeDeclHandler : NULL);
}

void XMLParser::EnableEndDoctypeDeclHandler(bool enable)
{
	assert(m_parser != NULL);
	XML_SetEndDoctypeDeclHandler(m_parser, enable ? EndDoctypeDeclHandler : NULL);
}

void XMLParser::EnableDoctypeDeclHandler(bool enable)
{
	assert(m_parser != NULL);
	EnableStartDoctypeDeclHandler(enable);
	EnableEndDoctypeDeclHandler(enable);
}

/************************************* Parser error reporting methods **/

enum XML_Error XMLParser::GetErrorCode(void) 
{
	assert(m_parser != NULL);
	return XML_GetErrorCode(m_parser);
}

long XMLParser::GetCurrentByteIndex(void) 
{
	assert(m_parser != NULL);
	return XML_GetCurrentByteIndex(m_parser);
}

int XMLParser::GetCurrentLineNumber(void) 
{
	assert(m_parser != NULL);
	return XML_GetCurrentLineNumber(m_parser);
}

int XMLParser::GetCurrentColumnNumber(void) 
{
	assert(m_parser != NULL);
	return XML_GetCurrentColumnNumber(m_parser);
}

int XMLParser::GetCurrentByteCount(void) 
{
	assert(m_parser != NULL);
	return XML_GetCurrentByteCount(m_parser);
}

const char* XMLParser::GetInputContext(int* pOffset, int* pSize)
{
	assert(m_parser != NULL);
	return XML_GetInputContext(m_parser, pOffset, pSize);
}

const XML_LChar* XMLParser::GetErrorString() 
{
	return XML_ErrorString(GetErrorCode());
}

/******************************************************* Parser Interface **/

const XML_LChar* XMLParser::GetExpatVersion(void)
{
	return XML_ExpatVersion();
}

const XML_LChar* XMLParser::GetErrorString(enum XML_Error err) 
{
	return XML_ErrorString(err);
}

/*********************************** Event Handler Default Implementation **/

XMLEvtHandler::XMLEvtHandler()
{
}

void XMLEvtHandler::OnPostCreate(void)
{
}

void XMLEvtHandler::OnStartElement(const XML_Char* name, const XML_Char** attrs)
{
}

void XMLEvtHandler::OnEndElement(const XML_Char* name)
{
}

void XMLEvtHandler::OnCharacterData(const XML_Char* data, int len)
{
}

void XMLEvtHandler::OnProcessingInstruction(const XML_Char* target, const XML_Char* data)
{
}

void XMLEvtHandler::OnComment(const XML_Char* data)
{
}

void XMLEvtHandler::OnStartCdataSection(void)
{
}

void XMLEvtHandler::OnEndCdataSection(void)
{
}

void XMLEvtHandler::OnDefault(const XML_Char* data, int len)
{
}

bool XMLEvtHandler::OnUnknownEncoding(const XML_Char* name, XML_Encoding* pInfo)
{
	return false;
}

void XMLEvtHandler::OnStartNamespaceDecl(const XML_Char* prefix, const XML_Char* uri)
{
}

void XMLEvtHandler::OnEndNamespaceDecl(const XML_Char* prefix)
{
}

void XMLEvtHandler::OnXmlDecl(const XML_Char* version, const XML_Char* encoding, bool isStandalone)
{
}

void XMLEvtHandler::OnStartDoctypeDecl(const XML_Char* doctype, const XML_Char* sysId, const XML_Char* pubId, bool hasInternalSubset)
{
}

void XMLEvtHandler::OnEndDoctypeDecl(void)
{
}
