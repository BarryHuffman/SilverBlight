// -------------------------------------------------------------------
// Copyright (c) Microsoft Corporation. All Rights Reserved.
// -------------------------------------------------------------------

using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Windows;
using System.Windows.Markup;
using System.Windows.Media.Animation;

namespace System.Windows
{
    /// <summary>
    /// Defines a transition between VisualStates.
    /// </summary>
    [ContentProperty("Storyboard")]
    public class VisualTransition : DependencyObject
    {
        public VisualTransition()
        {
            DynamicStoryboardCompleted = true;
            ExplicitStoryboardCompleted = true;
        }

        /// <summary>
        /// Name of the state to transition from.
        /// </summary>
        public string From
        {
            get;
            set;
        }

        /// <summary>
        /// Name of the state to transition to.
        /// </summary>
        public string To
        {
            get;
            set;
        }

        /// <summary>
        /// Storyboard providing fine grained control of the transition.
        /// </summary>
        public Storyboard Storyboard
        {
            get;
            set;
        }

        /// <summary>
        /// Duration of the transition.
        /// </summary>
        [TypeConverter(typeof(System.Windows.DurationConverter))]
        public Duration GeneratedDuration
        {
            get { return _generatedDuration; }
            set { _generatedDuration = value; }
        }


	public IEasingFunction GeneratedEasingFunction { get; set; }


        internal bool IsDefault
        {
            get { return From == null && To == null; }
        }

        internal bool DynamicStoryboardCompleted
        {
            get;
            set;
        }

        internal bool ExplicitStoryboardCompleted
        {
            get;
            set;
        }

        private Duration _generatedDuration = new Duration(new TimeSpan());
    }
}
