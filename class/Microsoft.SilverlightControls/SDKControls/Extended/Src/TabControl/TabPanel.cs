// (c) Copyright Microsoft Corporation.
// This source is subject to the Microsoft Public License (Ms-PL).
// Please see http://go.microsoft.com/fwlink/?LinkID=131993 for details.
// All other rights reserved.

using System.Windows.Media;

namespace System.Windows.Controls.Primitives
{
    /// <summary>
    /// TabPanel is a Panel designed to handle the intricacies of laying out the tab buttons in a TabControl.  Specically, it handles:
    /// Serving as an ItemsHost for TabItems within a TabControl 
    /// Determining correct sizing and positioning for TabItems 
    /// Handling the logic associated with MultiRow scenarios, namely: 
    /// Calculating row breaks in a collection of TabItems 
    /// Laying out TabItems in multiple rows based on those breaks 
    /// Performing specific layout for a selected item to indicate selection, namely: 
    /// Bringing the selected tab to the front, or, in other words, making the selected tab appear to be in front of other tabs. 
    /// Increasing the size pre-layout size of a selected item (note that this is not a transform, but rather an increase in the size allotted to the element in which to perform layout). 
    /// Bringing the selected tab to the front 
    /// Exposing attached properties that allow TabItems to be styled based on their placement within the TabPanel. 
    /// </summary>
    public class TabPanel : Panel
    {
        #region Constructor

        /// <summary>
        /// The default constructor for the TabPanel
        /// </summary>
        public TabPanel()
        {
        }

        #endregion Constructor

        #region Measure/Arrange

        /// <summary>
        /// Updates DesiredSize of the TabPanel.  Called by parent UIElement.  This is the first pass of layout.
        /// </summary>
        /// <param name="availableSize">AvailableSize size is an "upper limit" that TabPanel should not exceed.</param>
        /// <returns>TabPanel' desired size.</returns>
        protected override Size MeasureOverride(Size availableSize)
        {
            Size contentSize = new Size();
            Dock tabAlignment = TabAlignment;

            _numRows = 1;
            _rowHeight = 0;

            // For top and bottom placement the panel flow its children to calculate the number of rows and
            // desired vertical size
            if (tabAlignment == Dock.Top || tabAlignment == Dock.Bottom)
            {
                int numInCurrentRow = 0;
                double currentRowWidth = 0;
                double maxRowWidth = 0;
                foreach (UIElement child in Children)
                {
                    // Helper measures child, and deals with Min, Max, and base Width & Height properties.
                    // Helper returns the size a child needs to take up (DesiredSize or property specified size).
                    child.Measure(availableSize);

                    if (child.Visibility == Visibility.Collapsed)
                    {
                        continue;
                    }

                    Size childSize = GetDesiredSizeWithoutMargin(child);

                    if (_rowHeight < childSize.Height)
                    {
                        _rowHeight = childSize.Height;
                    }
                    if (currentRowWidth + childSize.Width > availableSize.Width && numInCurrentRow > 0)
                    { // If child does not fit in the current row - create a new row
                        if (maxRowWidth < currentRowWidth)
                        {
                            maxRowWidth = currentRowWidth;
                        }
                        currentRowWidth = childSize.Width;
                        numInCurrentRow = 1;
                        _numRows++;
                    }
                    else
                    {
                        currentRowWidth += childSize.Width;
                        numInCurrentRow++;
                    }
                }

                if (maxRowWidth < currentRowWidth)
                {
                    maxRowWidth = currentRowWidth;
                }
                contentSize.Height = _rowHeight * _numRows;

                // If we don't have constraint or content width is smaller than constraint width then size to content
                if (double.IsInfinity(contentSize.Width) || double.IsNaN(contentSize.Width) || maxRowWidth < availableSize.Width)
                {
                    contentSize.Width = maxRowWidth;
                }
                else
                {
                    contentSize.Width = availableSize.Width;
                }
            }
            else if (tabAlignment == Dock.Left || tabAlignment == Dock.Right)
            {
                foreach (UIElement child in Children)
                {
                    if (child.Visibility == Visibility.Collapsed)
                    {
                        continue;
                    }

                    // Helper measures child, and deals with Min, Max, and base Width & Height properties.
                    // Helper returns the size a child needs to take up (DesiredSize or property specified size).
                    child.Measure(availableSize);

                    Size childSize = GetDesiredSizeWithoutMargin(child);

                    if (contentSize.Width < childSize.Width)
                    {
                        contentSize.Width = childSize.Width;
                    }

                    contentSize.Height += childSize.Height;
                }
            }

            // Returns our minimum size & sets DesiredSize.
            return contentSize;
        }

        /// <summary>
        /// TabPanel arranges each of its children.
        /// </summary>
        /// <param name="finalSize">Size that TabPanel will assume to position children.</param>
        protected override Size ArrangeOverride(Size finalSize)
        {
            Dock tabAlignment = TabAlignment;
            if (tabAlignment == Dock.Top || tabAlignment == Dock.Bottom)
            {
                ArrangeHorizontal(finalSize);
            }
            else if (tabAlignment == Dock.Left || tabAlignment == Dock.Right)
            {
                ArrangeVertical(finalSize);
            }
            return finalSize;
        }

        private void ArrangeHorizontal(Size arrangeSize)
        {
            Dock tabAlignment = TabAlignment;
            bool isMultiRow = _numRows > 1;
            int activeRow = 0;
            int[] solution = new int[0];
            Size childOffset = new Size(0,0);
            double[] headerSize = GetHeadersSize();

            // If we have multirows, then calculate the best header distribution
            if (isMultiRow)
            {
                solution = CalculateHeaderDistribution(arrangeSize.Width, headerSize);
                activeRow = GetActiveRow(solution);

                // TabPanel starts to layout children depend on activeRow which should be always on bottom (top)
                // The first row should start from Y = (_numRows - 1 - activeRow) * _rowHeight
                if (tabAlignment == Dock.Top)
                {
                    childOffset.Height = (_numRows - 1 - activeRow) * _rowHeight;
                }
                if (tabAlignment == Dock.Bottom && activeRow != 0)
                {
                    childOffset.Height = (_numRows - activeRow) * _rowHeight;
                }
            }

            int childIndex = 0;
            int separatorIndex = 0;
            foreach (UIElement child in Children)
            {
                Thickness margin = (Thickness)child.GetValue(MarginProperty);
                double leftOffset = margin.Left;
                double rightOffset = margin.Right;
                double topOffset = margin.Top;
                double bottomOffset = margin.Bottom;

                bool lastHeaderInRow = isMultiRow && (separatorIndex < solution.Length && solution[separatorIndex] == childIndex || childIndex == Children.Count - 1);

                // Length left, top, right, bottom;
                Size cellSize = new Size(headerSize[childIndex], _rowHeight);

                // Align the last header in the row; If headers are not aligned directional navigation would not work correctly
                if (lastHeaderInRow)
                {
                    cellSize.Width = arrangeSize.Width - childOffset.Width;
                }

                // Set ZIndex
                TabItem tabItem = child as TabItem;
                if (tabItem != null)
                {
                    if (tabItem.IsSelected)
                    {
                        tabItem.SetValue(Canvas.ZIndexProperty, 1);
                    }
                    else
                    {
                        tabItem.SetValue(Canvas.ZIndexProperty, 0);
                    }
                }

                child.Arrange(new Rect(childOffset.Width, childOffset.Height, cellSize.Width, cellSize.Height));

                Size childSize = cellSize;
                childSize.Height = Math.Max(0d, childSize.Height - topOffset - bottomOffset);
                childSize.Width = Math.Max(0d, childSize.Width - leftOffset - rightOffset);

                // Calculate the offset for the next child
                childOffset.Width += cellSize.Width;
                if (lastHeaderInRow)
                {
                    if ((separatorIndex == activeRow && tabAlignment == Dock.Top) ||
                        (separatorIndex == activeRow - 1 && tabAlignment == Dock.Bottom))
                    {
                        childOffset.Height = 0d;
                    }
                    else
                    {
                        childOffset.Height += _rowHeight;
                    }

                    childOffset.Width = 0d;
                    separatorIndex++;
                }

                childIndex++;
            }
        }

        private void ArrangeVertical(Size arrangeSize)
        {
            double childOffsetY = 0d;
            foreach (UIElement child in Children)
            {
                if (child.Visibility != Visibility.Collapsed)
                {
                    // Set ZIndex
                    TabItem tabItem = child as TabItem;
                    if (tabItem != null)
                    {
                        if (tabItem.IsSelected)
                        {
                            tabItem.SetValue(Canvas.ZIndexProperty, 1);
                        }
                        else
                        {
                            tabItem.SetValue(Canvas.ZIndexProperty, 0);
                        }
                    }

                    Size childSize = GetDesiredSizeWithoutMargin(child);
                    child.Arrange(new Rect(0, childOffsetY, arrangeSize.Width, childSize.Height));

                    // Calculate the offset for the next child
                    childOffsetY += childSize.Height;
                }
            }
        }

        #endregion Measure/Arrange

        #region HelperFunctions

        private Dock TabAlignment
        {
            get { return TabControlParent != null ? TabControlParent.TabStripPlacement : Dock.Top; }
        }

        internal static Size GetDesiredSizeWithoutMargin(UIElement element)
        {
            // We have a hard coded margin value on each side for the
            // selected item. To account for this additional size,
            // we add this size to the calculations taking place.
            double selectedMarginSize = 0;
            TabItem tabItem = element as TabItem;
            if (tabItem != null && tabItem.IsSelected)
            {
                Panel panel = tabItem.GetTemplate(tabItem.IsSelected, tabItem.TabStripPlacement) as Panel;
                FrameworkElement fe = (panel != null && panel.Children.Count > 0) ? panel.Children[0] as FrameworkElement : null;
                if (fe != null)
                {
                    selectedMarginSize += (Math.Abs(fe.Margin.Left + fe.Margin.Right));
                }
            }

            Thickness margin = (Thickness)element.GetValue(MarginProperty);
            Size desiredSizeWithoutMargin = new Size();
            desiredSizeWithoutMargin.Height = Math.Max(0d, element.DesiredSize.Height - margin.Top - margin.Bottom);
            desiredSizeWithoutMargin.Width = Math.Max(0d, element.DesiredSize.Width - margin.Left - margin.Right + selectedMarginSize);
            return desiredSizeWithoutMargin;
        }

        // Returns the row which contain the child with IsSelected==true
        private int GetActiveRow(int[] solution)
        {
            int activeRow = 0;
            int childIndex = 0;
            if (solution.Length > 0)
            {
                foreach (UIElement child in Children)
                {
                    bool isActiveTab = (bool)child.GetValue(TabItem.IsSelectedProperty);

                    if (isActiveTab)
                    {
                        return activeRow;
                    }

                    if (activeRow < solution.Length && solution[activeRow] == childIndex)
                    {
                        activeRow++;
                    }

                    childIndex++;
                }
            }

            // If the is no selected element and aligment is Top  - then the active row is the last row 
            if (TabAlignment == Dock.Top)
            {
                activeRow = _numRows - 1;
            }

            return activeRow;
        }

        /*   TabPanel layout calculation:
         
        After measure call we have:
        rowWidthLimit - width of the TabPanel
        Header[0..n-1]  - headers
        headerWidth[0..n-1] - header width
         
        Calculated values:
        numSeparators                       - number of separators between numSeparators+1 rows
        rowWidth[0..numSeparators]           - row width
        rowHeaderCount[0..numSeparators]    - Row Count = number of headers on that row
        rowAverageGap[0..numSeparators]     - Average Gap for the row i = (rowWidth - rowWidth[i])/rowHeaderCount[i]
        currentSolution[0..numSeparators-1] - separator currentSolution[i]=x means Header[x] and h[x+1] are separated with new line
        bestSolution[0..numSeparators-1]    - keep the last Best Solution
        bestSolutionRowAverageGap           - keep the last Best Solution Average Gap

        Between all separators distribution the best solution have minimum Average Gap - 
        this is the amount of pixels added to the header (to justify) in the row

        How does it work:
        First we flow the headers to calculate the number of necessary rows (numSeparators+1).
        That means we need to insert numSeparators separators between n headers (numSeparators<n always).
        For each current state rowAverageGap[1..numSeparators+1] are calculated for each row.
        Current state rowAverageGap = MAX (rowAverageGap[1..numSeparators+1]).
        Our goal is to find the solution with MIN (rowAverageGap).
        On each iteration step we move a header from a previous row to the row with maximum rowAverageGap.
        We countinue the itterations only if we move to better solution, i.e. rowAverageGap is smaller.
        Maximum iteration steps are less the number of headers.

        */
        // Input: Row width and width of all headers
        // Output: int array which size is the number of separators and contains each separator position
        private int[] CalculateHeaderDistribution(double rowWidthLimit, double[] headerWidth)
        {
            double bestSolutionMaxRowAverageGap = 0;
            int numHeaders = headerWidth.Length;

            int numSeparators = _numRows - 1;
            double currentRowWidth = 0;
            int numberOfHeadersInCurrentRow = 0;
            double currentAverageGap = 0;
            int[] currentSolution = new int[numSeparators];
            int[] bestSolution = new int[numSeparators];
            int[] rowHeaderCount = new int[_numRows];
            double[] rowWidth = new double[_numRows];
            double[] rowAverageGap = new double[_numRows];
            double[] bestSolutionRowAverageGap = new double[_numRows];

            // Initialize the current state; Do the initial flow of the headers
            int currentRowIndex = 0;

            for (int index = 0; index < numHeaders; index++)
            {
                if (currentRowWidth + headerWidth[index] > rowWidthLimit && numberOfHeadersInCurrentRow > 0)
                { // if we cannot add next header - flow to next row
                    // Store current row before we go to the next
                    rowWidth[currentRowIndex] = currentRowWidth; // Store the current row width
                    rowHeaderCount[currentRowIndex] = numberOfHeadersInCurrentRow; // For each row we store the number os headers inside
                    currentAverageGap = Math.Max(0d, (rowWidthLimit - currentRowWidth) / numberOfHeadersInCurrentRow); // The amout of width that should be added to justify the header
                    rowAverageGap[currentRowIndex] = currentAverageGap;
                    currentSolution[currentRowIndex] = index - 1; // Separator points to the last header in the row
                    if (bestSolutionMaxRowAverageGap < currentAverageGap) // Remember the maximum of all currentAverageGap
                    {
                        bestSolutionMaxRowAverageGap = currentAverageGap;
                    }

                    // Iterate to next row
                    currentRowIndex++;
                    currentRowWidth = headerWidth[index]; // Accumulate header widths on the same row
                    numberOfHeadersInCurrentRow = 1;
                }
                else
                {
                    currentRowWidth += headerWidth[index]; // Accumulate header widths on the same row
                    // Increase the number of headers only if they are not collapsed (width=0)
                    if (headerWidth[index] != 0)
                    {
                        numberOfHeadersInCurrentRow++;
                    }
                }
            }

            // If everything fits in 1 row then exit (no separators needed)
            if (currentRowIndex == 0)
            {
                return new int[0];
            }

            // Add the last row
            rowWidth[currentRowIndex] = currentRowWidth;
            rowHeaderCount[currentRowIndex] = numberOfHeadersInCurrentRow;
            currentAverageGap = (rowWidthLimit - currentRowWidth) / numberOfHeadersInCurrentRow;
            rowAverageGap[currentRowIndex] = currentAverageGap;
            if (bestSolutionMaxRowAverageGap < currentAverageGap)
            {
                bestSolutionMaxRowAverageGap = currentAverageGap;
            }

            currentSolution.CopyTo(bestSolution, 0); // Remember the first solution as initial bestSolution
            rowAverageGap.CopyTo(bestSolutionRowAverageGap, 0); // bestSolutionRowAverageGap is used in ArrangeOverride to calculate header sizes

            // Search for the best solution
            // The exit condition if when we cannot move header to the next row 
            while (true)
            {
                // Find the row with maximum AverageGap
                int worstRowIndex = 0; // Keep the row index with maximum AverageGap
                double maxAG = 0;

                for (int i = 0; i < _numRows; i++) // for all rows
                {
                    if (maxAG < rowAverageGap[i])
                    {
                        maxAG = rowAverageGap[i];
                        worstRowIndex = i;
                    }
                }

                // If we are on the first row - cannot move from previous
                if (worstRowIndex == 0)
                {
                    break;
                }

                // From the row with maximum AverageGap we try to move a header from previous row
                int moveToRow = worstRowIndex;
                int moveFromRow = moveToRow - 1;
                int moveHeader = currentSolution[moveFromRow];
                double movedHeaderWidth = headerWidth[moveHeader];

                rowWidth[moveToRow] += movedHeaderWidth;

                // If the moved header cannot fit - exit. We have the best solution already.
                if (rowWidth[moveToRow] > rowWidthLimit)
                {
                    break;
                }

                // If header is moved successfully to the worst row
                // we update the arrays keeping the row state
                currentSolution[moveFromRow]--;
                rowHeaderCount[moveToRow]++;
                rowWidth[moveFromRow] -= movedHeaderWidth;
                rowHeaderCount[moveFromRow]--;
                rowAverageGap[moveFromRow] = (rowWidthLimit - rowWidth[moveFromRow]) / rowHeaderCount[moveFromRow];
                rowAverageGap[moveToRow] = (rowWidthLimit - rowWidth[moveToRow]) / rowHeaderCount[moveToRow];

                // EvaluateSolution:
                // If the current solution is better than bestSolution - keep it in bestSolution
                maxAG = 0;
                for (int i = 0; i < _numRows; i++) // for all rows
                {
                    if (maxAG < rowAverageGap[i])
                    {
                        maxAG = rowAverageGap[i];
                    }
                }

                if (maxAG < bestSolutionMaxRowAverageGap)
                {
                    bestSolutionMaxRowAverageGap = maxAG;
                    currentSolution.CopyTo(bestSolution, 0);
                    rowAverageGap.CopyTo(bestSolutionRowAverageGap, 0);
                }
            }

            // Each header size should be increased so headers in the row stretch to fit the row
            currentRowIndex = 0;
            for (int index = 0; index < numHeaders; index++)
            {
                headerWidth[index] += bestSolutionRowAverageGap[currentRowIndex];
                if (currentRowIndex < numSeparators && bestSolution[currentRowIndex] == index)
                {
                    currentRowIndex++;
                }
            }
            // Use the best solution bestSolution[0..numSeparators-1] to layout
            return bestSolution;
        }

        internal double[] GetHeadersSize()
        {
            double[] headerSize = new double[Children.Count];
            int childIndex = 0;
            foreach (UIElement child in Children)
            {
                Size childSize = GetDesiredSizeWithoutMargin(child);
                headerSize[childIndex] = child.Visibility == Visibility.Collapsed ? 0 : childSize.Width;
                childIndex++;
            }
            return headerSize;
        }

        #endregion HelperFunctions

        #region TabControlParent

        /// <summary>
        /// This will step up the UI tree to find the TabControl
        /// that contains this TabPanel.
        /// </summary>
        internal TabControl TabControlParent
        {
            get
            {
                FrameworkElement fe = this as FrameworkElement;
                while (fe != null)
                {
                    TabControl tc = fe as TabControl;
                    if (tc != null)
                    {
                        return tc;
                    }
                    fe = VisualTreeHelper.GetParent(fe) as FrameworkElement;
                }
                return null;
            }
        }

        #endregion TabControlParent
        
        #region Member Variables

        /// <summary>
        /// Number of row calculated in measure and used in arrange
        /// </summary>
        internal int _numRows = 1;

        /// <summary>
        /// Maximum of all headers height
        /// </summary>
        internal double _rowHeight;

        #endregion Member Variables
    }
}
