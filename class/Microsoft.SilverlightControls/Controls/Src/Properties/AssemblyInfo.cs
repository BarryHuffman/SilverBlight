// Copyright © Microsoft Corporation. 
// This source is subject to the Microsoft Source License for Silverlight Controls (March 2008 Release).
// Please see http://go.microsoft.com/fwlink/?LinkID=111693 for details.
// All other rights reserved. 

using System;
using System.Reflection; 
using System.Resources; 
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices; 
using System.Windows.Markup;

// General Information about an assembly is controlled through the following 
// set of attributes. Change these attribute values to modify the information
// associated with an assembly.
[assembly: AssemblyTitle("System.Windows.Controls")] 
[assembly: AssemblyDescription("")] 
[assembly: AssemblyConfiguration("")]
[assembly: AssemblyCompany("Microsoft")] 
[assembly: AssemblyProduct("System.Windows.Controls")]
[assembly: AssemblyCopyright("Copyright © Microsoft 2007")]
[assembly: AssemblyTrademark("")] 
[assembly: AssemblyCulture("")]
[assembly: NeutralResourcesLanguage("en-us")]
 
// 
[assembly: CLSCompliant(false)]
 
// Provide unit tests with access to internal members
#if DEBUG
[assembly: InternalsVisibleTo("Test, PublicKey=00240000048000009400000006020000002400005253413100040000010001000fed916f5fe67a46ed73e4151623c8572c7ffdd000cda61e0142afdf721bbd11b7d77367a6a9a5f20e1a930b1f480d0499280e63ab23e052692cdd117da8158ebe924d6b91d05a64a1b96fd87f3259e9223dcff8fe5915540e5386982227310de2a3cb8f78e78062bfe4012fac307a507f5a89d76d19c9845cf62fa6693d7cde")] 
#else
[assembly: InternalsVisibleTo("Test, PublicKey=0024000004800000940000000602000000240000525341310004000001000100b5fc90e7027f67871e773a8fde8938c81dd402ba65b9201d60593e96c492651e889cc13f1415ebb53fac1131ae0bd333c5ee6021672d9718ea31a8aebd0da0072f25d87dba6fc90ffd598ed4da35e44c398c454307e8e33b8426143daec9f596836f97c8f74750e5975c64e2189f45def46b2a2b1247adc3652bf5c308055da9")]
#endif 
 
// Setting ComVisible to false makes the types in this assembly not visible
// to COM components.  If you need to access a type in this assembly from 
// COM, set the ComVisible attribute to true on that type.
[assembly: ComVisible(false)]
 
// The following GUID is for the ID of the typelib if this project is exposed to COM
[assembly: Guid("172e8091-9d5f-4646-8125-7ec20c62e6e7")]
 
// Version information for an assembly consists of the following four values: 
//
//      Major Version 
//      Minor Version
//      Build Number
//      Revision 
//
// You can specify all the values or you can default the Revision and Build Numbers
// by using the '*' as shown below: 
[assembly: AssemblyVersion("1.0.0.0")] 
[assembly: AssemblyFileVersion("1.0.0.0")]
 
[assembly: XmlnsDefinition("http://schemas.microsoft.com/client/2007", "System.Windows.Controls")]
[assembly: XmlnsDefinition("http://schemas.microsoft.com/winfx/2006/xaml/presentation", "System.Windows.Controls")]
[assembly: XmlnsDefinition("http://schemas.microsoft.com/client/2007", "System.Windows.Controls.Primitives")] 
[assembly: XmlnsDefinition("http://schemas.microsoft.com/winfx/2006/xaml/presentation", "System.Windows.Controls.Primitives")]
