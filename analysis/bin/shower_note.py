# Select files in folder
# Code from: https://codereview.stackexchange.com/questions/162920/file-selection-button-for-jupyter-notebook

import pandas as pd
import numpy as np
import matplotlib as mp
import matplotlib.pyplot as plt
import matplotlib as mpl
import plotly.offline as py
import plotly.tools as tls
import plotly.graph_objs as go
import plotly.io as pio
import datetime as dt
pio.templates.default = "none" # IMPORTANT, otherwise plotly would use it's own (ugly) style set
import ipywidgets as widgets
from ipywidgets import interact, interact_manual, interactive, fixed

# Control figure size
mpl.rcParams['figure.figsize']=(7,5)
py.init_notebook_mode(connected=False)
# Use plotly as gifure output
def plotly_show():
    fig = plt.gcf()
    py.iplot_mpl(fig, strip_style= False, verbose=False)

import traitlets
from ipywidgets import widgets
from IPython.display import display
from tkinter import Tk, filedialog

class SelectFilesButton(widgets.Button):
    """A file widget that leverages tkinter.filedialog."""

    def __init__(self):
        super(SelectFilesButton, self).__init__()
        # Add the selected_files trait
        self.add_traits(files=traitlets.traitlets.List())
        # Create the button.
        self.description = "Select Files"
        self.icon = "square-o"
        self.style.button_color = "orange"
        # Set on click behavior.
        self.on_click(self.select_files)

    @staticmethod
    def select_files(b):
        """Generate instance of tkinter.filedialog.

        Parameters
        ----------
        b : obj:
            An instance of ipywidgets.widgets.Button 
        """
        with out:
            try:
                # Create Tk root
                root = Tk()
                # Hide the main window
                root.withdraw()
                # Raise the root to the top of all windows.
                root.call('wm', 'attributes', '.', '-topmost', True)
                # List of selected fileswill be set to b.value
                b.files = filedialog.askopenfilename(multiple=True)

                b.description = "Files Selected"
                b.icon = "check-square-o"
                b.style.button_color = "lightgreen"
            except:
                pass
out = widgets.Output()

# Color definition from coolors.co
colors = {'Fall_rgb': ['rgb(85,5,39)', 'rgb(104,142,38)'],
          'Fall_rgba': ['rgba(85,5,39,0.3)', 'rgba(104,142,38,0.3)']
         }
colors_df = pd.DataFrame(colors)

# Plot Time difference distribution as a scatter plot
# Options to pass to functions:
#   - dataframe (fixed): dataframe containing the chosen data
#   - name (drop_down): filename in dataframe
#   - conv_data (check_box): convert the times into datetime format (time consuming, only check for final plot)
#   - t_cut (drop_down): time cut in [us] for highlighting coincident events
#   - yrange (drop_down): pre-selectable y-axis scaling in [us]
def scatplot(dataframe,
             name,
             conv_date = False,
             t_cut = 10,
             yrange = 100
            ):
    
    #coincidence cut
    data = dataframe[name]
    t_cut = t_cut*1e3 # in ns
    data_cut = data[(data.tdiff <= t_cut) & (data.tdiff >= -t_cut)]
    # Prepare data for highlighting the time cut
    cut_upper = np.full(len(data.ts),t_cut*1e0)
    cut_lower = np.full(len(data.ts),-t_cut*1e0)
    cut_lower = cut_lower[::-1]
    x_rev = data.ts[::-1]
    x_prepared = np.append(data.ts, x_rev)
    # Convert the timestamps to datetime format for plotting
    if conv_date:
        xdata1 = [(dt.datetime.fromtimestamp(data.ts[index])) for index, rows in data.ts.iteritems()] 
        xdata2 = [(dt.datetime.fromtimestamp(data_cut.ts[index])) for index, rows in data_cut.ts.iteritems()]
        xdata3 = [(dt.datetime.fromtimestamp(x_prepared[i])) for i in np.arange(0,len(x_prepared),1)]
    else: 
        xdata1 = data.ts
        xdata2 = data_cut.ts
        xdata3 = x_prepared
    # Uncut data
    trace1 = go.Scattergl( # scattergl for increased speed!
        x = xdata1, 
        y = data.tdiff,
        mode = 'markers',
        marker_color = colors_df['Fall_rgb'][1],
        marker_size = 5,
        name = "Uncut"
    )
    # Cut data
    trace2 = go.Scattergl( # scattergl for increased speed!
        x = xdata2,
        y = data_cut.tdiff,
        mode = 'markers',
#         marker_color = 'rgb(0,176,246)',
        marker_color = colors_df['Fall_rgb'][0],
        marker_size = 10,
        name = "Cut"
    )
    trace3 = go.Scatter(
        x = xdata3,
        y = np.append(cut_upper,cut_lower),
        fill='toself',
        fillcolor=colors_df['Fall_rgba'][0],
        line_color='rgba(255,255,255,0)',
        showlegend=False,
        name='Fair',
    )
    
    # Data to plot
    plot_data = [trace1, trace2, trace3]

    layout = dict(title = 'Time Distribution Plot',
                  yaxis = dict(zeroline = True, mirror=True, ticks='outside', showline=True, 
                               showexponent = 'all', exponentformat = 'e', range=[-yrange*1e3, yrange*1e3], 
                               title="Time Difference [ns]"),
                  xaxis = dict(zeroline = True, mirror=True, ticks='outside', showline=True,
                               title = "Unix Timestamp", type = "date"),
                  xaxis_rangeslider_visible=False,
                  xaxis_tickformat = '%d %B <br>%Y<br>%H:%M:%S'
                 )

    fig = dict(data=plot_data, layout=layout)
        
    py.iplot(fig)