// (c) Copyright Microsoft Corporation.
// This source is subject to the Microsoft Public License (Ms-PL).
// Please see http://go.microsoft.com/fwlink/?LinkID=131993 for details.
// All other rights reserved.

using System.Windows.Automation.Provider;
using System.Windows.Controls;

namespace System.Windows.Automation.Peers
{
    /// <summary>
    /// AutomationPeer for DatePicker Control
    /// </summary>
    public sealed class DatePickerAutomationPeer : FrameworkElementAutomationPeer, IExpandCollapseProvider, IValueProvider
    {
        /// <summary>
        /// Initializes a new instance of the AutomationPeer for DatePicker control.
        /// </summary>
        /// <param name="owner">DatePicker</param>
        public DatePickerAutomationPeer(DatePicker owner)
            :base(owner)
        {

        }

        #region Private Properties

        private DatePicker OwningDatePicker
        {
            get
            {
                return this.Owner as DatePicker;
            }
        }

        #endregion Private Properties

        #region Public Methods

        /// <summary>
        /// Gets the control pattern that is associated with the specified System.Windows.Automation.Peers.PatternInterface.
        /// </summary>
        /// <param name="patternInterface">A value from the System.Windows.Automation.Peers.PatternInterface enumeration.</param>
        /// <returns>The object that supports the specified pattern, or null if unsupported.</returns>
        public override object GetPattern(PatternInterface patternInterface)
        {
            if (patternInterface == PatternInterface.ExpandCollapse || patternInterface == PatternInterface.Value)
            {
                return this;
            }
            return base.GetPattern(patternInterface);
        }

        #endregion Public Methods

        #region Protected Methods

        /// <summary>
        /// Gets the control type for the element that is associated with the UI Automation peer.
        /// </summary>
        /// <returns>The control type.</returns>
        protected override AutomationControlType GetAutomationControlTypeCore()
        {
            return AutomationControlType.ComboBox;
        }

        /// <summary>
        /// Called by GetClassName that gets a human readable name that, in addition to AutomationControlType, 
        /// differentiates the control represented by this AutomationPeer.
        /// </summary>
        /// <returns>The string that contains the name.</returns>
        protected override string GetClassNameCore()
        {
            return Owner.GetType().Name;
        }

        /// <summary>
        /// Overrides the GetLocalizedControlTypeCore method for DatePicker
        /// </summary>
        /// <returns></returns>
        protected override string GetLocalizedControlTypeCore()
        {
            return Resource.DatePickerAutomationPeer_LocalizedControlType;
        }

        /// <summary>
        /// 
        /// </summary>
        /// <returns></returns>
        protected override string GetNameCore()
        {
            string nameCore = base.GetNameCore();

            if (string.IsNullOrEmpty(nameCore))
            {
                AutomationPeer labeledByCore = this.GetLabeledByCore();
                if (labeledByCore != null)
                {
                    nameCore = labeledByCore.GetName();
                }
                if (string.IsNullOrEmpty(nameCore))
                {
                    nameCore = this.OwningDatePicker.ToString();
                }

            }
            return nameCore;
        }

        #endregion Protected Methods

        #region IExpandCollapseProvider

        ExpandCollapseState IExpandCollapseProvider.ExpandCollapseState
        {
            get
            {
                if (this.OwningDatePicker.IsDropDownOpen)
                {
                    return ExpandCollapseState.Expanded;
                }
                else
                {
                    return ExpandCollapseState.Collapsed;
                }
            }
        }

        void IExpandCollapseProvider.Collapse()
        {
            this.OwningDatePicker.IsDropDownOpen = false;
        }

        void IExpandCollapseProvider.Expand()
        {
            this.OwningDatePicker.IsDropDownOpen = true;
        }

        #endregion IExpandCollapseProvider

        #region IValueProvider

        bool IValueProvider.IsReadOnly { get { return false; } }

        string IValueProvider.Value { get { return this.OwningDatePicker.ToString(); } }

        void IValueProvider.SetValue(string value)
        {
            this.OwningDatePicker.Text = value;
        }

        #endregion IValueProvider
    }
}
