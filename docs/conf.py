# docs/conf.py
project = 'Cobalt'
copyright = '2026, Your Name'
author = 'Your Name'

extensions = [
    'myst_parser', # Allows writing docs in plain .md Markdown
]

templates_path = ['_templates']
exclude_patterns = ['_build', 'Thumbs.db', '.DS_Store']

html_theme = 'sphinx_rtd_theme'