/**
  src/base/omx_base_audio_port.c

  Base Audio Port class for OpenMAX ports to be used in derived components.

  Copyright (C) 2007-2009 STMicroelectronics
  Copyright (C) 2007-2009 Nokia Corporation and/or its subsidiary(-ies).

  This library is free software; you can redistribute it and/or modify it under
  the terms of the GNU Lesser General Public License as published by the Free
  Software Foundation; either version 2.1 of the License, or (at your option)
  any later version.

  This library is distributed in the hope that it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
  FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
  details.

  You should have received a copy of the GNU Lesser General Public License
  along with this library; if not, write to the Free Software Foundation, Inc.,
  51 Franklin St, Fifth Floor, Boston, MA
  02110-1301  USA

  $Date$
  Revision $Rev$
  Author $Author$
*/

#include <string.h>
#include <unistd.h>
#include <omxcore.h>
#include <OMX_Core.h>
#include <OMX_Component.h>

#include "omx_base_component.h"
#include "omx_base_audio_port.h"

/**
  * @brief The base constructor for the generic OpenMAX ST Audio port
  *
  * This function is executed by the component that uses a port.
  * The parameter contains the info about the component.
  * It takes care of constructing the instance of the port and
  * every object needed by the base port.
  *
  * @param openmaxStandComp pointer to the Handle of the component
  * @param openmaxStandPort the ST port to be initialized
  * @param nPortIndex Index of the port to be constructed
  * @param isInput specifies if the port is an input or an output
  *
  * @return OMX_ErrorInsufficientResources if a memory allocation fails
  */

OMX_ERRORTYPE base_audio_port_Constructor(OMX_COMPONENTTYPE *openmaxStandComp,omx_base_PortType **openmaxStandPort,OMX_U32 nPortIndex, OMX_BOOL isInput) {
	OMX_ERRORTYPE err;
	omx_base_audio_PortType *omx_base_audio_Port;

  DEBUG(DEB_LEV_FUNCTION_NAME, "In %s of component %x\n", __func__, (int)openmaxStandComp);
  if (!(*openmaxStandPort)) {
    *openmaxStandPort = calloc(1, sizeof(omx_base_audio_PortType));
  }

  if (!(*openmaxStandPort)) {
    return OMX_ErrorInsufficientResources;
  }

  err = base_port_Constructor(openmaxStandComp,openmaxStandPort,nPortIndex, isInput);
  if (err != OMX_ErrorNone) {
	  DEBUG(DEB_LEV_ERR, "In %s base port constructor failed\n", __func__);
	  return err;
  }

  omx_base_audio_Port = (omx_base_audio_PortType *)*openmaxStandPort;

  setHeader(&omx_base_audio_Port->sAudioParam, sizeof(OMX_AUDIO_PARAM_PORTFORMATTYPE));
  omx_base_audio_Port->sAudioParam.nPortIndex = nPortIndex;
  omx_base_audio_Port->sAudioParam.nIndex = 0;
  omx_base_audio_Port->sAudioParam.eEncoding = OMX_AUDIO_CodingUnused;

  omx_base_audio_Port->sPortParam.eDomain = OMX_PortDomainAudio;
  omx_base_audio_Port->sPortParam.format.audio.cMIMEType = malloc(DEFAULT_MIME_STRING_LENGTH);
  if (!omx_base_audio_Port->sPortParam.format.audio.cMIMEType) {
	  DEBUG(DEB_LEV_ERR, "Memory allocation failed in %s\n", __func__);
	  return OMX_ErrorInsufficientResources;
  }
  strcpy(omx_base_audio_Port->sPortParam.format.audio.cMIMEType, "raw/audio");
  omx_base_audio_Port->sPortParam.format.audio.pNativeRender = 0;
  omx_base_audio_Port->sPortParam.format.audio.bFlagErrorConcealment = OMX_FALSE;
  omx_base_audio_Port->sPortParam.format.audio.eEncoding = OMX_AUDIO_CodingUnused;

  omx_base_audio_Port->sPortParam.nBufferSize = (isInput == OMX_TRUE)?DEFAULT_IN_BUFFER_SIZE:DEFAULT_OUT_BUFFER_SIZE ;

  omx_base_audio_Port->PortDestructor = &base_audio_port_Destructor;

  DEBUG(DEB_LEV_FUNCTION_NAME, "Out of %s of component %x\n", __func__, (int)openmaxStandComp);
  return OMX_ErrorNone;
}

/**
  * @brief The base audio port destructor for the generic OpenMAX ST Audio port
  *
  * This function is executed by the component that uses a port.
  * The parameter contains the info about the port.
  * It takes care of destructing the instance of the port
  *
  * @param openmaxStandPort the ST port to be destructed
  *
  * @return OMX_ErrorNone
  */

OMX_ERRORTYPE base_audio_port_Destructor(omx_base_PortType *openmaxStandPort){
	OMX_ERRORTYPE err;
	DEBUG(DEB_LEV_FUNCTION_NAME, "In %s of port %x\n", __func__, (int)openmaxStandPort);
	if(openmaxStandPort->sPortParam.format.audio.cMIMEType) {
		free(openmaxStandPort->sPortParam.format.audio.cMIMEType);
		openmaxStandPort->sPortParam.format.audio.cMIMEType = NULL;
	}
	err = base_port_Destructor(openmaxStandPort);
	if (err != OMX_ErrorNone) {
		DEBUG(DEB_LEV_ERR, "In %s base port destructor failed\n", __func__);
		return err;
	}
	DEBUG(DEB_LEV_FUNCTION_NAME, "Out of %s of port %x\n", __func__, (int)openmaxStandPort);
	return OMX_ErrorNone;
}
