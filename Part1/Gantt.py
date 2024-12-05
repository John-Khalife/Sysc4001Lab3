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
    Get the color for an event. If the event is 'IDLE', return gray.
    Otherwise, assign a unique color to the event from the color pool.
    """
    if event_name == "IDLE":
        return "gray"
    if event_name not in event_color_mapping:
        # Assign a new color from the pool, ensuring no duplicate assignments
        color = color_pool.pop(0)
        event_color_mapping[event_name] = color
    return event_color_mapping[event_name]

def drawGantt(events, divisions, title, metrics):
    """
    Draws a Gantt chart with performance metrics, dynamic color mapping,
    a legend, and labels for transition points below the graph in a diagonal orientation.
    
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

    # Create a set of unique events to build the legend
    unique_events = set(event['Event'] for event in events)

    # Add invisible traces to serve as the legend
    for event_name in unique_events:
        event_color = get_event_color(event_name)
        fig.add_trace(go.Scatter(
            x=[None],  # Invisible trace
            y=[None],
            mode='markers',
            marker=dict(size=10, color=event_color),
            name=event_name,  # Add event name to the legend
            showlegend=True  # Ensure it appears in the legend
        ))

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
            name=row['Event'],
            showlegend=False  # Do not duplicate legend for individual blocks
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

    # Add transition point labels below the x-axis, rotated diagonally
    for point in divisions:
        fig.add_trace(go.Scatter(
            x=[point],  # Transition point
            y=[-0.1],  # Y position below the graph
            mode='text',
            text=f"{point}",  # Label with the transition time
            textposition='middle center',
            #textangle=45,  # Rotate text to be diagonal
            showlegend=False  # No legend for the transition points
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
            title="Time (seconds)",  # Title for x-axis
            showgrid=False,     # No grid lines
            zeroline=False,     # No zero line
            showline=True,      # Show axis line
            tickvals=[],        # No automatic ticks
            ticktext=[]         # No automatic tick text
        ),
        showlegend=True,  # Enable legend display
        title_x=0.5,  # Center the title
        paper_bgcolor='white',  # Set the outer background color to white
        plot_bgcolor='white',   # Set the plot area background color to white
    )

    # Show the plot
    fig.show()
