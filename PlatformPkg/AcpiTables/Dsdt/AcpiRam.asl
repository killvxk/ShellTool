/*++
  This file contains a 'Sample Driver' and is licensed as such  
  under the terms of your license agreement with Intel or your  
  vendor.  This file may be modified by the user, subject to    
  the additional terms of the license agreement                 
--*/

  // Define a Global region of ACPI NVS Region that may be used for any
  // type of implementation.  The starting offset and size will be fixed
  // up by the System BIOS during POST.  Note that the Size must be a word
  // in size to be fixed up correctly.

  OperationRegion(GNVS,SystemMemory,0xFFFF0000,0xAA55)
  Field(GNVS,AnyAcc,Lock,Preserve)
  {
      Offset(8),    // Signature
      FLSZ, 32,     // FlashSize
      MBB,  32,     // PciBase
      MBL,  32,     // PciLength      
      OSYS, 16,     // OSVersion
      IOST, 16,     // IoStates
      MXDW, 8,      // AcpiWakeState
      RTCS, 8,      // IsRtcWake
  }
