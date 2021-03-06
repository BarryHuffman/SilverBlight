// (c) Copyright Microsoft Corporation.
// This source is subject to [###LICENSE_NAME###].
// Please see [###LICENSE_LINK###] for details.
// All other rights reserved.

using System;
using System.Reflection;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Resources;
using System.Windows.Markup;

// Allow access to internal APIs for testing purposes
//#if DEBUG
//[assembly: InternalsVisibleTo("Test, PublicKey=0024000004800000940000000602000000240000525341310004000001000100b5fc90e7027f67871e773a8fde8938c81dd402ba65b9201d60593e96c492651e889cc13f1415ebb53fac1131ae0bd333c5ee6021672d9718ea31a8aebd0da0072f25d87dba6fc90ffd598ed4da35e44c398c454307e8e33b8426143daec9f596836f97c8f74750e5975c64e2189f45def46b2a2b1247adc3652bf5c308055da9")]
//[assembly: InternalsVisibleTo("Controls.Data.TestHooks, PublicKey=00240000048000009400000006020000002400005253413100040000010001000fed916f5fe67a46ed73e4151623c8572c7ffdd000cda61e0142afdf721bbd11b7d77367a6a9a5f20e1a930b1f480d0499280e63ab23e052692cdd117da8158ebe924d6b91d05a64a1b96fd87f3259e9223dcff8fe5915540e5386982227310de2a3cb8f78e78062bfe4012fac307a507f5a89d76d19c9845cf62fa6693d7cde")]

[assembly: XmlnsPrefix("clr-namespace:System.Windows.Controls;assembly=System.Windows.Controls.Data", "data")]
[assembly: XmlnsDefinitionAttribute("clr-namespace:System.Windows.Controls;assembly=System.Windows.Controls.Data", "System.Windows.Controls")]

//#else
[assembly: InternalsVisibleTo("Test, PublicKey=0024000004800000940000000602000000240000525341310004000001000100b5fc90e7027f67871e773a8fde8938c81dd402ba65b9201d60593e96c492651e889cc13f1415ebb53fac1131ae0bd333c5ee6021672d9718ea31a8aebd0da0072f25d87dba6fc90ffd598ed4da35e44c398c454307e8e33b8426143daec9f596836f97c8f74750e5975c64e2189f45def46b2a2b1247adc3652bf5c308055da9")]
[assembly: InternalsVisibleTo("Controls.Data.TestHooks, PublicKey=0024000004800000940000000602000000240000525341310004000001000100b5fc90e7027f67871e773a8fde8938c81dd402ba65b9201d60593e96c492651e889cc13f1415ebb53fac1131ae0bd333c5ee6021672d9718ea31a8aebd0da0072f25d87dba6fc90ffd598ed4da35e44c398c454307e8e33b8426143daec9f596836f97c8f74750e5975c64e2189f45def46b2a2b1247adc3652bf5c308055da9")]
//#endif

// The following GUID is for the ID of the typelib if this project is exposed to COM
[assembly: Guid("3d5900ae-111a-45be-96b3-d9e4606ca793")]
