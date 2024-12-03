import plotly.graph_objects as go
import pandas as pd
import random

# Initialize a global dictionary to store assigned colors for events
event_color_mapping = {}

# Define a pool of colors to assign to events dynamically
color_pool = [
    "blue", "orange", "green", "red", "purple", "cyan", "magenta", 
    "yellow", "lime", "pink", "teal", "brown", "gold", "navy", "olive"
]

def get_event_color(event_name):
    """
    Get the color for an event. If the event is 'IO', return gray.
    Otherwise, assign a unique color to the event from the color pool.
    """
    if event_name == "IO":
        return "gray"
    if event_name not in event_color_mapping:
        # Assign a new color from the pool, ensuring no duplicate assignments
        color = color_pool.pop(0)
        event_color_mapping[event_name] = color
    return event_color_mapping[event_name]

def drawGantt(events, divisions, title, metrics):
    """
    Draws a Gantt chart with performance metrics and dynamic color mapping.
    
    Parameters:
    events (list): List of events for the Gantt chart.
    divisions (list): List of division points corresponding to the events.
    title (str): Title of the chart.
    metrics (dict): Dictionary containing performance metrics.
    """
    # Create a DataFrame from events
    df = pd.DataFrame(events)

    # Create a figure
    fig = go.Figure()

    # Loop through the events to draw the Gantt bars
    for index, row in df.iterrows():
        # Get the start and end from the divisions based on the index
        start = divisions[index]
        end = divisions[index + 1] if index + 1 < len(divisions) else start + 1  # Set default end if no next division

        # Get or assign the color for the event
        event_color = get_event_color(row['Event'])

        # Calculate the midpoint for label placement
        midpoint = (start + end) / 2
        
        # Add filled rectangle for the event
        fig.add_trace(go.Scatter(
            x=[start, end, end, start, start],  # Create a closed shape
            y=[0, 0, 1, 1, 0],  # y-coordinates for the rectangle (full height)
            fill='toself',      # Fill the area under the line
            line=dict(width=0), # No border line
            fillcolor=event_color,  # Fill color from the color mapping
            name=row['Event']
        ))

        # Add event name as text in the middle of the block
        fig.add_trace(go.Scatter(
            x=[midpoint],  # Use the midpoint for the x position
            y=[0.5],       # Set the y value to the middle (0.5)
            mode='text',
            text=row['Event'],
            textposition='middle center',  # Position text in the middle of the block
            showlegend=False,  # Hide the legend for the text traces
            textfont=dict(
                size=16,  # Set font size
                color='white'  # Set text color to white
            )
        ))

    # Add performance metrics as a text annotation
    metrics_text = (
        f"Throughput: {metrics['throughput']} processes/sec\n"
        f"Average Wait Time: {metrics['avg_wait_time']:.2f} sec\n"
        f"Average Turnaround Time: {metrics['avg_turnaround_time']:.2f} sec\n"
        f"Average Response Time: {metrics['avg_response_time']:.2f} sec"
    )
    fig.add_annotation(
        text=metrics_text,
        xref="paper", yref="paper",
        x=1.05, y=1.1,  # Position in the corner
        showarrow=False,
        font=dict(size=12, color="black"),
        align="left",
        bordercolor="black",
        borderwidth=1,
        bgcolor="white",
    )

    # Update layout
    fig.update_layout(
        title=title,  # Title
        yaxis=dict(visible=False),  # Hide y-axis completely
        xaxis=dict(
            title= "Time (seconds)",  # Title for x-axis
            visible=True,      # Hide the axis completely (no ticks or lines)
            showgrid=False,     # No grid lines
            zeroline=False,     # No zero line
            showline=False,     # No axis line
            tickvals=[],        # No tick values
            ticktext=[]        # No tick text
        ),
        showlegend=False,
        title_x=0.5,  # Center the title
        title_y=0.8,  # Adjust distance of title from the graph
        paper_bgcolor='white',  # Set the outer background color to white
        plot_bgcolor='white',   # Set the plot area background color to white
    )

    # Add markers for the division points
    for point in divisions:
        fig.add_trace(go.Scatter(
            x=[point,point],  # Division point
            y=[0,1],      # Y position for marker
            mode='markers+text+lines',  # Show marker and text
            marker=dict(size=10, color='black'),  # Marker style
            line=dict(color='black', width=2), 
            textfont=dict(
                size=12,  # Set font size
                color='black'  # Set text color to black
            ),
            text=[point],  # Marker label
            textposition='bottom center',  # Position label below marker
            showlegend=False,  # Hide legend for this trace
        ))

    # Show the plot
    fig.show()
